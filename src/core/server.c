#include "core/server.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "core/epoll.h"
#include "core/worker.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/net.h"

static void cws_server_setup_hints(struct addrinfo *hints, const char *hostname) {
	memset(hints, 0, sizeof *hints);

	hints->ai_family = AF_UNSPEC;
	hints->ai_socktype = SOCK_STREAM;

	if (hostname == NULL) {
		hints->ai_flags = AI_PASSIVE;
	}
}

static bool cws_server_is_listening(const cws_server_s *server, int fd) {
	for (size_t i = 0; i < server->sockfd_count; ++i) {
		if (server->sockfds[i] == fd) {
			return true;
		}
	}

	return false;
}

static cws_return cws_server_setup_sockets(cws_server_s *server, struct addrinfo *res) {
	const int opt = 1;

	size_t count = 0;
	for (struct addrinfo *rp = res; rp; rp = rp->ai_next) {
		++count;
	}

	server->sockfds = malloc(count * sizeof *server->sockfds);
	if (!server->sockfds) {
		return CWS_SOCKET_ERROR;
	}

	for (size_t i = 0; i < count; ++i) {
		server->sockfds[i] = -1;
	}

	/* Try every resolved address and keep the ones that bind and listen */
	for (struct addrinfo *rp = res; rp; rp = rp->ai_next) {
		int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd < 0) {
			cws_log_warning("socket(): %s", strerror(errno));
			continue;
		}

		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) != 0) {
			cws_log_warning("setsockopt(): %s", strerror(errno));
			close(fd);
			continue;
		}

		if (bind(fd, rp->ai_addr, rp->ai_addrlen) != 0) {
			cws_log_warning("bind(): %s", strerror(errno));
			close(fd);
			continue;
		}

		if (listen(fd, CWS_SERVER_BACKLOG) != 0) {
			cws_log_warning("listen(): %s", strerror(errno));
			close(fd);
			continue;
		}

		server->sockfds[server->sockfd_count++] = fd;
	}

	if (server->sockfd_count == 0) {
		return CWS_BIND_ERROR;
	}

	return CWS_OK;
}

cws_return cws_server_setup(cws_server_s *server, cws_config_s *config) {
	if (!config || !config->host || !config->port) {
		return CWS_CONFIG_ERROR;
	}

	server->epfd = -1;
	server->sockfds = NULL;
	server->sockfd_count = 0;

	cws_return returncode = CWS_OK;

	struct addrinfo hints = {0};
	struct addrinfo *res = NULL;
	cws_server_setup_hints(&hints, config->host);

	int status = getaddrinfo(config->host, config->port, &hints, &res);
	if (status != 0) {
		cws_log_error("getaddrinfo() error: %s", gai_strerror(status));
		returncode = CWS_GETADDRINFO_ERROR;
		goto cleanup;
	}

	returncode = cws_server_setup_sockets(server, res);
	if (returncode != CWS_OK) {
		goto cleanup;
	}

	server->epfd = epoll_create1(0);
	if (server->epfd < 0) {
		returncode = CWS_EPOLL_CREATE_ERROR;
		goto cleanup;
	}

	for (size_t i = 0; i < server->sockfd_count; ++i) {
		cws_return ret = cws_fd_set_nonblocking(server->sockfds[i]);
		if (ret != CWS_OK) {
			returncode = ret;
			goto cleanup;
		}

		if (cws_epoll_add(server->epfd, server->sockfds[i]) != 0) {
			returncode = CWS_EPOLL_CREATE_ERROR;
			goto cleanup;
		}
	}

	server->workers = cws_worker_new(config->workers, config);
	if (server->workers == NULL) {
		returncode = CWS_WORKER_ERROR;
		goto cleanup;
	}

	return CWS_OK;

cleanup:
	if (res) {
		freeaddrinfo(res);
	}

	if (server->sockfds) {
		for (size_t i = 0; i < server->sockfd_count; ++i) {
			if (server->sockfds[i] >= 0) {
				close(server->sockfds[i]);
			}
		}
		free(server->sockfds);
		server->sockfds = NULL;
		server->sockfd_count = 0;
	}

	if (server->epfd >= 0) {
		close(server->epfd);
		server->epfd = -1;
	}

	return returncode;
}

cws_return cws_server_start(cws_server_s *server) {
	struct epoll_event events[128];
	memset(events, 0, sizeof events);

	size_t workers_index = 0;

	while (cws_server_run) {
		int nfds = epoll_wait(server->epfd, events, CWS_SERVER_EPOLL_MAXEVENTS, CWS_SERVER_EPOLL_TIMEOUT);

		if (nfds <= 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (!cws_server_is_listening(server, events[i].data.fd)) {
				continue;
			}

			int client_fd = cws_server_handle_new_client(events[i].data.fd);
			if (client_fd < 0) {
				continue;
			}

			if (cws_fd_set_nonblocking(client_fd) != CWS_OK) {
				close(client_fd);
				continue;
			}

			if (cws_epoll_add(server->workers[workers_index]->epfd, client_fd) != 0) {
				close(client_fd);
				continue;
			}
			workers_index = (workers_index + 1) % server->config->workers;
		}
	}

	return CWS_OK;
}

int cws_server_handle_new_client(int server_fd) {
	struct sockaddr_storage their_sa;
	char ip[INET6_ADDRSTRLEN];

	int client_fd = cws_server_accept_client(server_fd, &their_sa);
	if (client_fd < 0) {
		return client_fd;
	}

	cws_utils_get_client_ip(&their_sa, ip);
	cws_log_info("Client (%s) (fd: %d) connected", ip, client_fd);

	return client_fd;
}

int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa) {
	socklen_t theirsa_size = sizeof(struct sockaddr_storage);

	const int client_fd = accept(server_fd, (struct sockaddr *)their_sa, &theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			cws_log_error("accept(): %s", strerror(errno));
		}
	}

	return client_fd;
}

void cws_server_shutdown(cws_server_s *server) {
	if (!server) {
		return;
	}

	if (server->sockfds) {
		for (size_t i = 0; i < server->sockfd_count; ++i) {
			if (server->sockfds[i] >= 0) {
				close(server->sockfds[i]);
			}
		}
		free(server->sockfds);
		server->sockfds = NULL;
	}

	if (server->epfd >= 0) {
		close(server->epfd);
	}

	if (server->workers) {
		cws_worker_free(server->workers, server->config->workers);
	}
}
