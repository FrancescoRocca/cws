#include "core/worker.h"

#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "core/epoll.h"
#include "core/socket.h"
#include "http/handler.h"
#include "http/request.h"
#include "http/response.h"
#include "utils/error.h"

/* Create epoll instance for a worker */
static cws_return worker_setup_epoll(cws_worker_s *worker) {
	worker->epfd = epoll_create1(0);
	if (worker->epfd == -1) {
		return CWS_EPOLL_CREATE_ERROR;
	}
	return CWS_OK;
}

/* Remove client from epoll and close socket */
static void worker_close_client(int epfd, int client_fd) {
	cws_epoll_del(epfd, client_fd);
	close(client_fd);
}

static cws_return worker_handle_client_data(int epfd, int client_fd) {
	string_s *data = string_new("", 4096);

	/* Read data from socket */
	ssize_t total_bytes = cws_socket_read(client_fd, data);

	if (total_bytes == 0) {
		/* Partial request; wait for more data */
		string_free(data);

		return CWS_OK;
	}

	if (total_bytes < 0) {
		worker_close_client(epfd, client_fd);
		string_free(data);
		return CWS_CLIENT_DISCONNECTED_ERROR;
	}

	/* Parse HTTP request */
	cws_request_s *request = cws_http_parse(data);
	string_free(data);
	if (request == NULL) {
		worker_close_client(epfd, client_fd);
		return CWS_HTTP_PARSE_ERROR;
	}

	/* Configure handler */
	cws_handler_config_s config = {.root_dir = "www", .index_file = "index.html"};

	/* Handle request and generate response */
	cws_response_s *response = cws_handler_static_file(request, &config);

	/* Send response */
	if (response) {
		cws_response_send(client_fd, response);
		cws_response_free(response);
	}

	/* Cleanup */
	cws_http_free(request);
	worker_close_client(epfd, client_fd);

	return CWS_OK;
}

/* Worker thread: process events on its epoll instance */
static void *cws_worker_loop(void *arg) {
	cws_worker_s *worker = arg;
	struct epoll_event events[64];

	while (cws_server_run) {
		/* 250 ms timeout allows periodic shutdown checking */
		int nfds = epoll_wait(worker->epfd, events, WORKER_EPOLL_MAX_EVENTS, WORKER_EPOLL_TIMEOUT);

		if (nfds <= 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			int client_fd = events[i].data.fd;
			worker_handle_client_data(worker->epfd, client_fd);
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
				free(workers[j]);
			}
			free(workers);
			return NULL;
		}
	}

	/* Start worker threads */
	for (size_t i = 0; i < workers_num; ++i) {
		pthread_create(&workers[i]->thread, NULL, cws_worker_loop, workers[i]);
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
		free(workers[i]);
	}

	free(workers);
}
