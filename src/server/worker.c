#include "server/worker.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "http/http.h"
#include "utils/debug.h"

static int cws_worker_setup_epoll(cws_worker_s *worker) {
	worker->epfd = epoll_create1(0);
	if (worker->epfd == -1) {
		return -1;
	}

	cws_server_ret ret;
	ret = cws_fd_set_nonblocking(worker->pipefd[0]);
	if (ret != CWS_SERVER_OK) {
		sock_close(worker->epfd);

		return -1;
	}

	ret = cws_epoll_add(worker->epfd, worker->pipefd[0]);
	if (ret != CWS_SERVER_OK) {
		sock_close(worker->epfd);
		sock_close(worker->pipefd[0]);
	}

	return 0;
}

static int cws_read_data(int sockfd, string_s *str) {
	char tmp[4096];
	memset(tmp, 0, sizeof(tmp));

	int bytes = sock_readall(sockfd, tmp, sizeof(tmp));
	string_append(str, tmp);

	return bytes;
}

cws_worker_s **cws_worker_new(size_t workers_num, cws_config_s *config) {
	cws_worker_s **workers = malloc(sizeof(cws_worker_s) * workers_num);
	if (workers == NULL) {
		return NULL;
	}
	memset(workers, 0, sizeof(cws_worker_s) * workers_num);

	for (size_t i = 0; i < workers_num; ++i) {
		workers[i] = malloc(sizeof(cws_worker_s));
		if (workers[i] == NULL) {
			for (size_t j = 0; j < i; ++j) {
				free(workers[j]);

				return NULL;
			}

			free(workers);
		}
		memset(workers[i], 0, sizeof(cws_worker_s));

		workers[i]->config = config;

		/* Communicate though threads */
		pipe(workers[i]->pipefd);
		int ret = cws_worker_setup_epoll(workers[i]);
		if (ret == -1) {
			for (size_t j = 0; j < i; ++j) {
				free(workers[j]);

				return NULL;
			}

			free(workers);
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
		nfds = epoll_wait(worker->epfd, events, 64, -1);

		if (nfds < 0) {
			continue;
		}

		if (nfds == 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == worker->pipefd[0]) {
				/* Handle new client */
				int client_fd;
				read(worker->pipefd[0], &client_fd, sizeof(int));
				cws_fd_set_nonblocking(client_fd);
				cws_epoll_add(worker->epfd, client_fd);
			} else {
				/* Handle client data */
				int client_fd = events[i].data.fd;
				cws_server_handle_client_data(worker->epfd, client_fd);
			}
		}
	}

	return NULL;
}

void cws_server_close_client(int epfd, int client_fd) {
	cws_epoll_del(epfd, client_fd);
	sock_close(client_fd);
}

cws_server_ret cws_epoll_add(int epfd, int sockfd) {
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_add()");
		return CWS_SERVER_EPOLL_ADD_ERROR;
	}

	return CWS_SERVER_OK;
}

cws_server_ret cws_epoll_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_del()");
		return CWS_SERVER_EPOLL_DEL_ERROR;
	}

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_handle_client_data(int epfd, int client_fd) {
	string_s *data = string_new("", 4096);

	size_t total_bytes = cws_read_data(client_fd, data);
	if (total_bytes <= 0) {
		if (data) {
			string_free(data);
		}
		cws_server_close_client(epfd, client_fd);

		return CWS_SERVER_CLIENT_DISCONNECTED_ERROR;
	}

	cws_http_s *request = cws_http_parse(data);
	request->sockfd = client_fd;

	string_free(data);

	// TODO: fix response

	if (request == NULL) {
		cws_server_close_client(epfd, client_fd);

		return CWS_SERVER_HTTP_PARSE_ERROR;
	}

	cws_http_free(request);

	return CWS_SERVER_OK;
}
