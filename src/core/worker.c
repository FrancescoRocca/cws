#include "core/worker.h"

#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "core/epoll.h"
#include "core/socket.h"
#include "http/request.h"
#include "http/response.h"
#include "utils/error.h"

static cws_return worker_setup_epoll(cws_worker_s *worker) {
	worker->epfd = epoll_create1(0);
	if (worker->epfd == -1) {
		return CWS_EPOLL_CREATE_ERROR;
	}

	return CWS_OK;
}

static void worker_close_client(int epfd, int client_fd) {
	cws_epoll_del(epfd, client_fd);
	close(client_fd);
}

static cws_return worker_read_data(int epfd, int client_fd, string_s *data) {
	ssize_t total_bytes = cws_read_data(client_fd, data);
	if (total_bytes == 0) {
		/* Request not completed yet */
		return CWS_OK;
	}

	if (total_bytes < 0) {
		/* Something happened, close connection */
		worker_close_client(epfd, client_fd);

		return CWS_CLIENT_DISCONNECTED_ERROR;
	}

	return CWS_OK;
}

static cws_return worker_handle_client_data(int epfd, int client_fd) {
	string_s *data = string_new("", 4096);
	cws_return ret = worker_read_data(epfd, client_fd, data);
	if (ret != CWS_OK) {
		string_free(data);
		return ret;
	}

	cws_request_s *request = cws_http_parse(data);
	string_free(data);
	if (request == NULL) {
		worker_close_client(epfd, client_fd);
		return CWS_HTTP_PARSE_ERROR;
	}
	request->sockfd = client_fd;

	cws_http_send_response(request, HTTP_OK);

	cws_http_free(request);
	worker_close_client(epfd, client_fd);

	return CWS_OK;
}

static void *cws_worker_loop(void *arg) {
	cws_worker_s *worker = arg;
	struct epoll_event events[64];

	int nfds;

	while (cws_server_run) {
		nfds = epoll_wait(worker->epfd, events, 64, 250);

		if (nfds < 0) {
			continue;
		}

		if (nfds == 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			/* Handle client's data */
			int client_fd = events[i].data.fd;
			worker_handle_client_data(worker->epfd, client_fd);
		}
	}

	return NULL;
}

cws_worker_s **cws_worker_new(size_t workers_num, cws_config_s *config) {
	cws_worker_s **workers = malloc(workers_num * sizeof *workers);
	if (workers == NULL) {
		return NULL;
	}
	memset(workers, 0, workers_num * sizeof *workers);

	for (size_t i = 0; i < workers_num; ++i) {
		workers[i] = malloc(sizeof(cws_worker_s));
		if (workers[i] == NULL) {
			for (size_t j = 0; j < i; ++j) {
				free(workers[j]);
			}

			free(workers);
			return NULL;
		}
		memset(workers[i], 0, sizeof **workers);

		workers[i]->config = config;

		/* Setup worker's epoll */
		int ret = worker_setup_epoll(workers[i]);
		if (ret == -1) {
			for (size_t j = 0; j < i; ++j) {
				free(workers[j]);
			}

			free(workers);
			return NULL;
		}
	}

	for (size_t i = 0; i < workers_num; ++i) {
		pthread_create(&workers[i]->thread, NULL, cws_worker_loop, workers[i]);
	}

	return workers;
}

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
