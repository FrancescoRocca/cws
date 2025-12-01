#include "core/server.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>

#include "core/epoll.h"
#include "core/worker.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/net.h"
#include <unistd.h>

/* Prepare addrinfo hints for getaddrinfo() */
static void cws_server_setup_hints(struct addrinfo *hints, const char *hostname) {
	memset(hints, 0, sizeof *hints);

	hints->ai_family = AF_UNSPEC;	  /* Accept IPv4 or IPv6 */
	hints->ai_socktype = SOCK_STREAM; /* TCP socket */

	if (hostname == NULL) {
		hints->ai_flags = AI_PASSIVE; /* Bind to all interfaces */
	}
}

/* Create epoll, set listening socket nonblocking, and register it */
static cws_return cws_server_setup_epoll(int server_fd, int *epfd_out) {
	int epfd = epoll_create1(0);
	if (epfd < 0) {
		return epfd;
	}

	/* Listening socket must be nonblocking for epoll-driven accept loop */
	cws_return ret = cws_fd_set_nonblocking(server_fd);
	if (ret != CWS_OK) {
		return ret;
	}

	cws_epoll_add(epfd, server_fd);
	*epfd_out = epfd;

	return CWS_OK;
}

/* Initialize listening socket, epoll instance, and worker threads */
cws_return cws_server_setup(cws_server_s *server, cws_config_s *config) {
	if (!config || !config->hostname || !config->port) {
		return CWS_CONFIG_ERROR;
	}

	memset(server, 0, sizeof *server);

	/* Resolve hostname/port */
	struct addrinfo hints;
	struct addrinfo *res;
	cws_server_setup_hints(&hints, config->hostname);

	int status = getaddrinfo(config->hostname, config->port, &hints, &res);
	if (status != 0) {
		CWS_LOG_ERROR("getaddrinfo() error: %s", gai_strerror(status));
		return CWS_GETADDRINFO_ERROR;
	}

	/* Create listening socket */
	server->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server->sockfd < 0) {
		CWS_LOG_ERROR("socket(): %s", strerror(errno));
		return CWS_SOCKET_ERROR;
	}

	/* Allow fast reuse of the port on restart */
	const int opt = 1;
	status = setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		CWS_LOG_ERROR("setsockopt(): %s", strerror(errno));
		return CWS_SETSOCKOPT_ERROR;
	}

	/* Bind + listen on the configured address */
	status = bind(server->sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		CWS_LOG_ERROR("bind(): %s", strerror(errno));
		return CWS_BIND_ERROR;
	}

	status = listen(server->sockfd, CWS_SERVER_BACKLOG);
	if (status != 0) {
		CWS_LOG_ERROR("listen(): %s", strerror(errno));
		return CWS_LISTEN_ERROR;
	}

	freeaddrinfo(res);

	/* Setup epoll for accepting new clients */
	cws_return ret = cws_server_setup_epoll(server->sockfd, &server->epfd);
	if (ret != CWS_OK) {
		return ret;
	}

	/* Spawn worker threads */
	server->workers = cws_worker_new(CWS_WORKERS_NUM, config);
	if (server->workers == NULL) {
		return CWS_WORKER_ERROR;
	}

	return CWS_OK;
}

/* Main event loop: accept clients and distribute them among workers */
cws_return cws_server_start(cws_server_s *server) {
	struct epoll_event events[128];
	memset(events, 0, sizeof events);

	size_t workers_index = 0;

	while (cws_server_run) {
		int nfds = epoll_wait(server->epfd, events, 128, -1);

		/* Epoll error */
		if (nfds < 0) {
			continue;
		}

		if (nfds == 0) {
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			/* Accept incoming connection */
			int client_fd = cws_server_handle_new_client(server->sockfd);
			if (client_fd < 0) {
				continue;
			}

			/* Assign client to next worker using round-robin */
			cws_fd_set_nonblocking(client_fd);
			cws_epoll_add(server->workers[workers_index]->epfd, client_fd);
			workers_index = (workers_index + 1) % CWS_WORKERS_NUM;
		}
	}

	return CWS_OK;
}

/* Accept client + log IP */
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

/* Wrapper around accept() with logging and error handling */
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

/* Close sockets, epoll, and workers */
void cws_server_shutdown(cws_server_s *server) {
	if (!server) {
		return;
	}

	if (server->sockfd > 0) {
		close(server->sockfd);
	}

	if (server->epfd > 0) {
		close(server->epfd);
	}

	if (server->workers) {
		cws_worker_free(server->workers, CWS_WORKERS_NUM);
	}
}
