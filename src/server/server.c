#include "server/server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

#include "http/http.h"
#include "server/threadpool.h"
#include "utils/colors.h"
#include "utils/utils.h"

volatile sig_atomic_t cws_server_run = 1;

static void cws_server_setup_hints(struct addrinfo *hints, const char *hostname) {
	memset(hints, 0, sizeof(struct addrinfo));

	/* IPv4 or IPv6 */
	hints->ai_family = AF_UNSPEC;

	/* TCP */
	hints->ai_socktype = SOCK_STREAM;

	if (hostname == NULL) {
		/* Fill in IP for me */
		hints->ai_flags = AI_PASSIVE;
	}
}

cws_server_ret cws_server_start(cws_config *config) {
	if (!config || !config->hostname || !config->port) {
		return CWS_SERVER_CONFIG;
	}

	struct addrinfo hints;
	struct addrinfo *res;

	cws_server_setup_hints(&hints, config->hostname);

	int status = getaddrinfo(config->hostname, config->port, &hints, &res);
	if (status != 0) {
		CWS_LOG_ERROR("getaddrinfo() error: %s", gai_strerror(status));
		return CWS_SERVER_GETADDRINFO_ERROR;
	}

	int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server_fd < 0) {
		CWS_LOG_ERROR("socket(): %s", strerror(errno));
		return CWS_SERVER_SOCKET_ERROR;
	}

	const int opt = 1;
	status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		CWS_LOG_ERROR("setsockopt(): %s", strerror(errno));
		return CWS_SERVER_SETSOCKOPT_ERROR;
	}

	status = bind(server_fd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		CWS_LOG_ERROR("bind(): %s", strerror(errno));
		return CWS_SERVER_BIND_ERROR;
	}

	status = listen(server_fd, CWS_SERVER_BACKLOG);
	if (status != 0) {
		CWS_LOG_ERROR("listen(): %s", strerror(errno));
		return CWS_SERVER_LISTEN_ERROR;
	}

	cws_server_ret ret = cws_server_loop(server_fd, config);
	CWS_LOG_DEBUG("cws_server_loop ret: %d", ret);

	freeaddrinfo(res);
	close(server_fd);

	return CWS_SERVER_OK;
}

static cws_server_ret cws_server_setup_epoll(int sockfd, int *epfd_out) {
	int epfd = epoll_create1(0);
	if (epfd == -1) {
		CWS_LOG_ERROR("epoll_create(): %s", strerror(errno));

		return CWS_SERVER_EPOLL_CREATE_ERROR;
	}

	cws_server_ret ret;

	ret = cws_fd_set_nonblocking(sockfd);
	if (ret != CWS_SERVER_OK) {
		close(epfd);

		return ret;
	}

	ret = cws_epoll_add(epfd, sockfd, EPOLLIN | EPOLLET);
	if (ret != CWS_SERVER_OK) {
		close(epfd);

		return ret;
	}

	*epfd_out = epfd;

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_loop(int server_fd, cws_config *config) {
	cws_server_ret ret;
	int epfd;

	ret = cws_server_setup_epoll(server_fd, &epfd);
	if (ret != CWS_SERVER_OK) {
		return ret;
	}

	mcl_hashmap *clients = mcl_hm_init(my_int_hash_fn, my_int_equal_fn, my_int_free_key_fn, my_str_free_fn, sizeof(int), sizeof(char) * INET_ADDRSTRLEN);
	if (!clients) {
		return CWS_SERVER_HASHMAP_INIT;
	}

	struct epoll_event *revents = malloc(CWS_SERVER_EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	if (!revents) {
		mcl_hm_free(clients);
		close(epfd);

		return CWS_SERVER_MALLOC_ERROR;
	}

	cws_threadpool *pool = cws_threadpool_init(4, 16);
	if (pool == NULL) {
		mcl_hm_free(clients);
		close(epfd);

		return CWS_SERVER_THREADPOOL_ERROR;
	}

	while (cws_server_run) {
		int nfds = epoll_wait(epfd, revents, CWS_SERVER_EPOLL_MAXEVENTS, CWS_SERVER_EPOLL_TIMEOUT);

		if (nfds == 0) {
			/* TODO: Check for inactive clients */
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == server_fd) {
				int client_fd = cws_server_handle_new_client(server_fd, epfd, clients);
				if (client_fd < 0) {
					continue;
				}
				cws_epoll_add(epfd, client_fd, EPOLLIN | EPOLLET);
				cws_fd_set_nonblocking(client_fd);
			} else {
				int client_fd = revents[i].data.fd;

				cws_task *task = malloc(sizeof(cws_task));
				if (!task) {
					cws_server_close_client(epfd, client_fd, clients);

					continue;
				}

				task->client_fd = client_fd;
				task->config = config;

				cws_thread_task *ttask = malloc(sizeof(cws_thread_task));
				if (!ttask) {
					free(task);

					continue;
				}

				ttask->arg = task;
				ttask->function = (void *)cws_server_handle_client_data;

				ret = cws_threadpool_submit(pool, ttask);
				free(ttask);
			}
		}
	}

	/* Clean up everything */
	free(revents);
	close(epfd);
	mcl_hm_free(clients);
	cws_threadpool_destroy(pool);

	return CWS_SERVER_OK;
}

int cws_server_handle_new_client(int server_fd, int epfd, mcl_hashmap *clients) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;
	char ip[INET_ADDRSTRLEN];

	int client_fd = cws_server_accept_client(server_fd, &their_sa, &theirsa_size);
	if (client_fd < 0) {
		return client_fd;
	}

	cws_utils_get_client_ip(&their_sa, ip);
	CWS_LOG_INFO("Client (%s) (fd: %d) connected", ip, client_fd);

	return client_fd;
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

void *cws_server_handle_client_data(void *arg) {
	cws_task *task = (cws_task *)arg;

	int client_fd = task->client_fd;
	cws_config *config = task->config;

	/* Read data from socket */
	mcl_string *data = NULL;
	size_t total_bytes = cws_read_data(client_fd, &data);
	if (total_bytes < 0) {
		mcl_string_free(data);
		free(task);

		/* TODO: Here we should close the client_fd, same as total == 0 */

		return NULL;
	}

	if (total_bytes == 0) {
		mcl_string_free(data);

		return NULL;
	}

	/* Parse HTTP request */
	// CWS_LOG_DEBUG("Raw request: %s", mcl_string_cstr(data));
	cws_http *request = cws_http_parse(data, client_fd, config);
	mcl_string_free(data);

	if (request == NULL) {
		return NULL;
	}

	/* TODO: fix keep-alive */
	int _keepalive = cws_http_send_resource(request);
	cws_http_free(request);

	/* Free the task */
	free(task);

	return NULL;
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

cws_server_ret cws_fd_set_nonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		CWS_LOG_ERROR("fcntl(): %s", strerror(errno));
		return CWS_SERVER_FD_NONBLOCKING_ERROR;
	}

	return CWS_SERVER_OK;
}

int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size) {
	const int client_fd = accept(server_fd, (struct sockaddr *)their_sa, theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			CWS_LOG_ERROR("accept(): %s", strerror(errno));
		}
		return -1;
	}

	return client_fd;
}

void cws_server_close_client(int epfd, int client_fd, mcl_hashmap *hashmap) {
	if (fcntl(client_fd, F_GETFD) != -1) {
		/* TODO: race condition here */
		cws_epoll_del(epfd, client_fd);
		mcl_hm_remove(hashmap, &client_fd);
	}
}
