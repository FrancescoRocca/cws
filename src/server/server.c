#include "server/server.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "server/worker.h"
#include "utils/debug.h"
#include "utils/utils.h"

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

static cws_server_ret cws_server_setup_epoll(int server_fd, int *epfd_out) {
	int epfd = epoll_create1(0);
	if (epfd < 0) {
		return epfd;
	}

	cws_server_ret ret;
	ret = cws_fd_set_nonblocking(server_fd);
	if (ret != CWS_SERVER_OK) {
		return ret;
	}

	cws_epoll_add(epfd, server_fd);
	*epfd_out = epfd;

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_setup(cws_server_s *server, cws_config_s *config) {
	if (!config || !config->hostname || !config->port) {
		return CWS_SERVER_CONFIG;
	}

	/* Setup basic stuff */
	struct addrinfo hints;
	struct addrinfo *res;

	cws_server_setup_hints(&hints, config->hostname);

	int status = getaddrinfo(config->hostname, config->port, &hints, &res);
	if (status != 0) {
		CWS_LOG_ERROR("getaddrinfo() error: %s", gai_strerror(status));
		return CWS_SERVER_GETADDRINFO_ERROR;
	}

	server->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server->sockfd < 0) {
		CWS_LOG_ERROR("socket(): %s", strerror(errno));
		return CWS_SERVER_SOCKET_ERROR;
	}

	const int opt = 1;
	status = setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		CWS_LOG_ERROR("setsockopt(): %s", strerror(errno));
		return CWS_SERVER_SETSOCKOPT_ERROR;
	}

	status = bind(server->sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		CWS_LOG_ERROR("bind(): %s", strerror(errno));
		return CWS_SERVER_BIND_ERROR;
	}

	status = listen(server->sockfd, CWS_SERVER_BACKLOG);
	if (status != 0) {
		CWS_LOG_ERROR("listen(): %s", strerror(errno));
		return CWS_SERVER_LISTEN_ERROR;
	}

	freeaddrinfo(res);

	/* Setup epoll */
	cws_server_ret ret = cws_server_setup_epoll(server->sockfd, &server->epfd);
	if (ret != CWS_SERVER_OK) {
		return ret;
	}

	/* Setup workers */
	cws_worker_s **workers = cws_worker_new(CWS_WORKERS_NUM, config);
	if (workers == NULL) {
		return CWS_SERVER_WORKER_ERROR;
	}

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_start(cws_server_s *server) {
	struct epoll_event events[128];
	memset(events, 0, sizeof(events));
	int client_fd = 0;
	size_t workers_index = 0;

	while (cws_server_run) {
		int nfds = epoll_wait(server->epfd, events, 128, -1);

		/* epoll error */
		if (nfds < 0) {
			continue;
		}

		/* No events */
		if (nfds == 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == server->sockfd) {
				client_fd = cws_server_handle_new_client(server->sockfd);
				if (client_fd < 0) {
					continue;
				}

				/* Add client to worker */
				write(server->workers[workers_index]->pipefd[1], &client_fd, sizeof(int));
				workers_index = (workers_index + 1) % CWS_WORKERS_NUM;
			}
		}
	}

	return CWS_SERVER_OK;
}

int cws_server_handle_new_client(int server_fd) {
	struct sockaddr_storage their_sa;
	char ip[INET_ADDRSTRLEN];

	int client_fd = cws_server_accept_client(server_fd, &their_sa);
	if (client_fd < 0) {
		return client_fd;
	}

	cws_utils_get_client_ip(&their_sa, ip);
	CWS_LOG_INFO("Client (%s) (fd: %d) connected", ip, client_fd);

	return client_fd;
}

int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa) {
	socklen_t theirsa_size = sizeof(struct sockaddr_storage);
	const int client_fd = accept(server_fd, (struct sockaddr *)their_sa, &theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			CWS_LOG_ERROR("accept(): %s", strerror(errno));
		}
	}

	return client_fd;
}

void cws_server_shutdown(cws_server_s *server) {
	if (!server) {
		return;
	}

	if (server->sockfd > 0) {
		sock_close(server->sockfd);
	}

	if (server->epfd > 0) {
		sock_close(server->epfd);
	}

	if (server->workers) {
		cws_worker_free(server->workers, CWS_WORKERS_NUM);
	}
}
