#include "server/worker.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "http/http.h"
#include "utils/colors.h"

static int cws_worker_setup_epoll(cws_worker *worker) {
	worker->epfd = epoll_create1(0);
	if (worker->epfd == -1) {
		return -1;
	}

	cws_server_ret ret;
	ret = cws_fd_set_nonblocking(worker->pipefd[0]);
	if (ret != CWS_SERVER_OK) {
		close(worker->epfd);

		return -1;
	}

	ret = cws_epoll_add(worker->epfd, worker->pipefd[0], EPOLLIN);
	if (ret != CWS_SERVER_OK) {
		close(worker->epfd);
		close(worker->pipefd[0]);
	}

	return 0;
}

cws_worker **cws_worker_init(size_t workers_num, mcl_hashmap *clients, cws_config *config) {
	cws_worker **workers = malloc(sizeof(cws_worker) * workers_num);
	if (workers == NULL) {
		return NULL;
	}
	memset(workers, 0, sizeof(cws_worker) * workers_num);

	for (size_t i = 0; i < workers_num; ++i) {
		workers[i] = malloc(sizeof(cws_worker));
		if (workers[i] == NULL) {
			for (size_t j = 0; j < i; ++j) {
				free(workers[j]);

				return NULL;
			}

			free(workers);
		}
		memset(workers[i], 0, sizeof(cws_worker));

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

void cws_worker_free(cws_worker **workers, size_t workers_num) {
	for (size_t i = 0; i < workers_num; ++i) {
		pthread_join(workers[i]->thread, NULL);
		free(workers[i]);
	}

	free(workers);
}

void *cws_worker_loop(void *arg) {
	cws_worker *worker = arg;
	struct epoll_event events[32];

	int nfds;

	while (cws_server_run) {
		nfds = epoll_wait(worker->epfd, events, 32, 1000);
		if (nfds == 0) {
			continue;
		}

		for (size_t i = 0; i < nfds; ++i) {
			if (events[i].data.fd == worker->pipefd[0]) {
				/* Handle new client */
				int client_fd;
				read(worker->pipefd[0], &client_fd, sizeof(int));
				CWS_LOG_DEBUG("Data from main, add client: %d", client_fd);
				cws_fd_set_nonblocking(client_fd);
				cws_epoll_add(worker->epfd, client_fd, EPOLLIN | EPOLLET);
			} else {
				/* Handle client data */
				int client_fd = events[i].data.fd;
				CWS_LOG_DEBUG("Data from client (thread: %ld)", worker->thread);
				cws_server_handle_client_data(worker->epfd, client_fd, worker->clients, worker->config);
			}
		}
	}

	return NULL;
}

void cws_server_close_client(int epfd, int client_fd, mcl_hashmap *clients) {
	mcl_bucket *client = mcl_hm_get(clients, &client_fd);
	if (client) {
		cws_epoll_del(epfd, client_fd);
		mcl_hm_remove(clients, &client_fd);
		mcl_hm_free_bucket(client);
	}
}

cws_server_ret cws_epoll_add(int epfd, int sockfd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_add(): %s", strerror(errno));
		return CWS_SERVER_EPOLL_ADD_ERROR;
	}

	return CWS_SERVER_OK;
}

cws_server_ret cws_epoll_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_del(): %s", strerror(errno));
		return CWS_SERVER_EPOLL_DEL_ERROR;
	}

	return CWS_SERVER_OK;
}

static size_t cws_read_data(int sockfd, mcl_string **str) {
	size_t total_bytes = 0;
	ssize_t bytes_read;

	if (*str == NULL) {
		*str = mcl_string_new("", 4096);
	}

	char tmp[4096];
	memset(tmp, 0, sizeof(tmp));

	while (1) {
		bytes_read = recv(sockfd, tmp, sizeof(tmp), 0);

		if (bytes_read == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}

			CWS_LOG_ERROR("recv(): %s", strerror(errno));

			return -1;
		} else if (bytes_read == 0) {
			return -1;
		}

		total_bytes += bytes_read;
		mcl_string_append(*str, tmp);
	}

	return total_bytes;
}

cws_server_ret cws_server_handle_client_data(int epfd, int client_fd, mcl_hashmap *clients, cws_config *config) {
	/* Read data from socket */
	mcl_string *data = NULL;
	size_t total_bytes = cws_read_data(client_fd, &data);
	if (total_bytes <= 0) {
		if (data) {
			mcl_string_free(data);
		}
		cws_server_close_client(epfd, client_fd, clients);

		return CWS_SERVER_CLIENT_DISCONNECTED_ERROR;
	}

	/* Parse HTTP request */
	cws_http *request = cws_http_parse(data, client_fd, config);
	mcl_string_free(data);

	if (request == NULL) {
		cws_server_close_client(epfd, client_fd, clients);

		return CWS_SERVER_HTTP_PARSE_ERROR;
	}

	int keepalive = cws_http_send_resource(request);
	cws_http_free(request);
	if (!keepalive) {
		cws_server_close_client(epfd, client_fd, clients);
	}

	return CWS_SERVER_OK;
}
