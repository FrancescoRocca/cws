#include "core/worker.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "core/epoll.h"
#include "core/socket.h"
#include "http/handler.h"
#include "http/request.h"
#include "http/response.h"
#include "internal/common.h"
#include "utils/debug.h"
#include "utils/error.h"

/* Create epoll instance for a worker */
static cws_return worker_setup_epoll(cws_worker_s *worker) {
	worker->epfd = epoll_create1(0);
	if (worker->epfd == -1) {
		return CWS_EPOLL_CREATE_ERROR;
	}
	return CWS_OK;
}

/* Look up a client by fd */
static cws_client_s *worker_find_client(cws_worker_s *worker, int fd) {
	for (size_t i = 0; i < worker->clients_len; ++i) {
		if (worker->clients[i].fd == fd) {
			return &worker->clients[i];
		}
	}

	return NULL;
}

/* Get (or create) the buffer for a client fd */
static cws_client_s *worker_get_client(cws_worker_s *worker, int fd) {
	cws_client_s *cl = worker_find_client(worker, fd);
	if (cl) {
		return cl;
	}

	if (worker->clients_len == worker->clients_cap) {
		size_t newcap = worker->clients_cap ? worker->clients_cap * 2 : 16;
		cws_client_s *grown = realloc(worker->clients, newcap * sizeof *grown);
		if (!grown) {
			return NULL;
		}
		worker->clients = grown;
		worker->clients_cap = newcap;
	}

	cl = &worker->clients[worker->clients_len++];
	cl->fd = fd;
	cl->buffer = string_new("", 4096);

	return cl;
}

/* Drop a client entry */
static void worker_remove_client(cws_worker_s *worker, int fd) {
	for (size_t i = 0; i < worker->clients_len; ++i) {
		if (worker->clients[i].fd == fd) {
			if (worker->clients[i].buffer) {
				string_free(worker->clients[i].buffer);
			}
			worker->clients[i] = worker->clients[worker->clients_len - 1];
			worker->clients_len--;
			return;
		}
	}
}

/* Remove client from epoll, close socket and drop buffered state */
static void worker_close_client(cws_worker_s *worker, int client_fd) {
	cws_epoll_del(worker->epfd, client_fd);
	close(client_fd);
	worker_remove_client(worker, client_fd);
}

static cws_return worker_handle_request(cws_worker_s *worker, int client_fd, string_s *buffer, size_t request_len,
										bool *close_connection) {
	*close_connection = false;

	char *raw = strndup(string_cstr(buffer), request_len);
	if (!raw) {
		*close_connection = true;
		return CWS_UNKNOWN_ERROR;
	}

	string_s *request_str = string_new(raw, request_len + 1);
	free(raw);

	cws_request_s *request = cws_request_parse(request_str);
	string_free(request_str);
	if (!request) {
		cws_log_warning("Malformed request from fd %d, closing connection", client_fd);
		*close_connection = true;
		return CWS_HTTP_PARSE_ERROR;
	}

	/* Configure handler virtual host */
	char *host = cws_request_get_header(request, "host");
	cws_vhost_s *vh = config_get_vhost(worker->config, host);
	cws_handler_config_s conf;
	if (vh) {
		conf = (cws_handler_config_s){.domain = vh->domain, .root = vh->root};
	} else {
		conf = (cws_handler_config_s){.domain = "default", .root = worker->config->root};
	}
	free(host);

	/* Handle request and generate response */
	cws_response_s *response = cws_handler_static_file(request, &conf);

	/* Send response */
	if (response) {
		cws_response_send(client_fd, response);
		/* Unsupported methods */
		if (response->status == HTTP_NOT_IMPLEMENTED) {
			*close_connection = true;
		}
		cws_response_free(response);
	}

	/* Connection keep-alive */
	const char *version = string_cstr(request->http_version);
	char *conn = cws_request_get_header(request, "Connection");
	bool close_requested = conn && !strcasecmp(conn, "close");
	bool keep_alive_requested = conn && !strcasecmp(conn, "keep-alive");
	free(conn);

	if (strstr(version, "HTTP/1.0")) {
		/* HTTP/1.0: default close, explicit keep-alive to stay open */
		if (!keep_alive_requested) {
			*close_connection = true;
		}
	} else {
		/* HTTP/1.1 (or unknown): default keep-alive */
		if (close_requested) {
			*close_connection = true;
		}
	}

	/* Cleanup */
	cws_request_free(request);

	return CWS_OK;
}

static void worker_handle_client_data(cws_worker_s *worker, int client_fd) {
	cws_client_s *cl = worker_get_client(worker, client_fd);
	if (!cl) {
		cws_epoll_del(worker->epfd, client_fd);
		close(client_fd);
		return;
	}

	/* Read data from socket, appending to the client buffer */
	int total_bytes = cws_socket_read(client_fd, cl->buffer);

	/* Connection closed or error */
	if (total_bytes == 0 || total_bytes == -1) {
		worker_close_client(worker, client_fd);
		return;
	}

	/* No data available yet */
	if (total_bytes == -2) {
		return;
	}

	/* Serve all complete requests currently buffered */
	for (;;) {
		int end = string_find(cl->buffer, "\r\n\r\n");
		if (end < 0) {
			/* Incomplete request: wait for more data (bounded by MAX_REQUEST_SIZE) */
			if (string_len(cl->buffer) > MAX_REQUEST_SIZE) {
				cws_log_warning("Request from fd %d exceeds size limit, closing", client_fd);
				worker_close_client(worker, client_fd);
			}
			return;
		}

		bool close_connection = false;
		cws_return ret = worker_handle_request(worker, client_fd, cl->buffer, (size_t)end + 4, &close_connection);
		string_remove(cl->buffer, 0, (size_t)end + 4);

		if (ret != CWS_OK || close_connection) {
			worker_close_client(worker, client_fd);
			return;
		}
	}
}

/* Worker thread: process events on its epoll instance */
static void *cws_worker_loop(void *arg) {
	cws_worker_s *worker = (cws_worker_s *)arg;
	struct epoll_event events[64];

	while (cws_server_run) {
		/* 250 ms timeout allows periodic shutdown checking */
		int nfds = epoll_wait(worker->epfd, events, WORKER_EPOLL_MAX_EVENTS, WORKER_EPOLL_TIMEOUT);

		if (nfds <= 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			worker_handle_client_data(worker, events[i].data.fd);
		}
	}

	return NULL;
}

/* Allocate workers, create per-worker epoll, then spawn worker threads */
cws_worker_s **cws_worker_new(size_t workers_num, cws_config_s *config) {
	cws_worker_s **workers = malloc(workers_num * sizeof *workers);
	if (!workers) {
		return NULL;
	}
	memset(workers, 0, workers_num * sizeof *workers);

	for (size_t i = 0; i < workers_num; ++i) {
		workers[i] = malloc(sizeof(cws_worker_s));
		if (!workers[i]) {
			for (size_t j = 0; j < i; ++j) {
				free(workers[j]);
			}
			free(workers);
			return NULL;
		}
		memset(workers[i], 0, sizeof **workers);

		workers[i]->config = config;

		/* Create per-worker epoll instance */
		if (worker_setup_epoll(workers[i]) != CWS_OK) {
			for (size_t j = 0; j < i; ++j) {
				close(workers[j]->epfd);
				free(workers[j]);
			}
			free(workers);
			return NULL;
		}
	}

	/* Start worker threads */
	for (size_t i = 0; i < workers_num; ++i) {
		if (pthread_create(&workers[i]->thread, NULL, cws_worker_loop, workers[i]) != 0) {
			for (size_t j = 0; j < i; ++j) {
				pthread_cancel(workers[j]->thread);
				pthread_join(workers[j]->thread, NULL);
				close(workers[j]->epfd);
				free(workers[j]);
			}
			free(workers);
			return NULL;
		}
	}

	return workers;
}

/* Join threads and free worker memory */
void cws_worker_free(cws_worker_s **workers, size_t workers_num) {
	if (!workers) {
		return;
	}

	for (size_t i = 0; i < workers_num; ++i) {
		pthread_join(workers[i]->thread, NULL);

		for (size_t j = 0; j < workers[i]->clients_len; ++j) {
			if (workers[i]->clients[j].buffer) {
				string_free(workers[i]->clients[j].buffer);
			}
		}
		free(workers[i]->clients);

		close(workers[i]->epfd);
		free(workers[i]);
	}

	free(workers);
}
