#include "server/worker.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "http/http.h"
#include "server/epoll_utils.h"
#include "utils/utils.h"

static cws_server_ret cws_worker_setup_epoll(cws_worker_s *worker) {
	worker->epfd = epoll_create1(0);
	if (worker->epfd == -1) {
		return CWS_SERVER_EPOLL_CREATE_ERROR;
	}

	return CWS_SERVER_OK;
}

static ssize_t cws_read_data(int sockfd, string_s *str) {
	char tmp[4096] = {0};

	ssize_t n = recv(sockfd, tmp, sizeof tmp, MSG_PEEK);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		return -1;
	}

	if (n == 0) {
		/* Connection closed */
		return -1;
	}

	string_append(str, tmp);
	return n;
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
		int ret = cws_worker_setup_epoll(workers[i]);
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

void *cws_worker_loop(void *arg) {
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
			cws_server_handle_client_data(worker->epfd, client_fd);
		}
	}

	return NULL;
}

void cws_server_close_client(int epfd, int client_fd) {
	cws_epoll_del(epfd, client_fd);
	close(client_fd);
}

cws_server_ret cws_server_handle_client_data(int epfd, int client_fd) {
	string_s *data = string_new("", 4096);

	ssize_t total_bytes = cws_read_data(client_fd, data);
	if (total_bytes == 0) {
		/* Request not completed yet */
		string_free(data);
		return CWS_SERVER_OK;
	}

	if (total_bytes <= 0) {
		/* Something happened, close connection */
		string_free(data);
		cws_server_close_client(epfd, client_fd);

		return CWS_SERVER_CLIENT_DISCONNECTED_ERROR;
	}

	cws_http_s *request = cws_http_parse(data);
	string_free(data);
	if (request == NULL) {
		cws_server_close_client(epfd, client_fd);
		return CWS_SERVER_HTTP_PARSE_ERROR;
	}
	request->sockfd = client_fd;

	cws_http_send_response(request, HTTP_OK);

	cws_http_free(request);
	cws_server_close_client(epfd, client_fd);

	return CWS_SERVER_OK;
}
