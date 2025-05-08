#include "server/server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

#include "http/http.h"
#include "utils/colors.h"
#include "utils/utils.h"

volatile bool cws_server_run = 1;

int cws_server_start(cws_config *config) {
	struct addrinfo hints;
	struct addrinfo *res;

	cws_server_setup_hints(&hints, sizeof hints, config->hostname);

	int status = getaddrinfo(config->hostname, config->port, &hints, &res);
	if (status != 0) {
		CWS_LOG_ERROR("getaddrinfo() error: %s", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		CWS_LOG_ERROR("socket(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	const int opt = 1;
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		CWS_LOG_ERROR("setsockopt(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		CWS_LOG_ERROR("bind(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	status = listen(sockfd, CWS_SERVER_BACKLOG);
	if (status != 0) {
		CWS_LOG_ERROR("listen(): %s", strerror(status));
		exit(EXIT_FAILURE);
	}

	cws_server_loop(sockfd, config);

	freeaddrinfo(res);
	close(sockfd);

	return 0;
}

void cws_server_setup_hints(struct addrinfo *hints, size_t len, const char *hostname) {
	memset(hints, 0, len);

	/* IPv4 or IPv6 */
	hints->ai_family = AF_UNSPEC;

	/* TCP */
	hints->ai_socktype = SOCK_STREAM;

	if (hostname == NULL) {
		/* Fill in IP for me */
		hints->ai_flags = AI_PASSIVE;
	}
}

void cws_server_loop(int sockfd, cws_config *config) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;

	cws_hashmap *clients = cws_hm_init(my_int_hash_fn, my_int_equal_fn, NULL, my_int_free_fn);

	int epfd = epoll_create1(0);
	cws_fd_set_nonblocking(sockfd);
	cws_epoll_add(epfd, sockfd, EPOLLIN | EPOLLET);

	struct epoll_event *revents = malloc(CWS_SERVER_EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	int client_fd;

	while (cws_server_run) {
		int nfds = epoll_wait(epfd, revents, CWS_SERVER_EPOLL_MAXEVENTS, CWS_SERVER_EPOLL_TIMEOUT);

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == sockfd) {
				/* New client */
				char ip[INET_ADDRSTRLEN];
				client_fd = cws_server_accept_client(sockfd, &their_sa, &theirsa_size);
				cws_utils_get_client_ip(&their_sa, ip);
				CWS_LOG_INFO("Client (%s) connected", ip);

				cws_fd_set_nonblocking(client_fd);
				cws_epoll_add(epfd, client_fd, EPOLLIN);
				cws_hm_set(clients, &client_fd, &their_sa);
			} else {
				char data[4096] = {0};
				char ip[INET_ADDRSTRLEN] = {0};

				/* Incoming data */
				client_fd = revents[i].data.fd;
				const ssize_t bytes_read = recv(client_fd, data, sizeof data, 0);

				/* Retrieve client ip */
				cws_bucket *client = cws_hm_get(clients, &client_fd);
				struct sockaddr_storage client_sas = *(struct sockaddr_storage *)client->value;
				cws_utils_get_client_ip(&client_sas, ip);

				if (bytes_read == 0) {
					/* Client disconnected */
					CWS_LOG_INFO("Client (%s) disconnected", ip);
					cws_server_close_client(epfd, client_fd, clients);
					continue;
				}
				if (bytes_read < 0) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						/* Error during read, handle it (close client) */
						cws_epoll_del(epfd, client_fd);
						close(client_fd);
					}
					continue;
				}

				data[bytes_read] = '\0';

				/* Parse HTTP request */
				cws_http *request = cws_http_parse(data, client_fd, config);

				if (request == NULL) {
					cws_server_close_client(epfd, client_fd, clients);
					cws_http_free(request);
					continue;
				}

				cws_http_send_response(request);
				CWS_LOG_INFO("Client (%s) disconnected", ip);
				cws_server_close_client(epfd, client_fd, clients);
				cws_http_free(request);

				/* Clear str */
				memset(data, 0, sizeof data);
			}
		}
	}

	/* Clean up everything */
	free(revents);
	close(epfd);
	cws_hm_free(clients);
	CWS_LOG_INFO("Closing...");
}

void cws_epoll_add(int epfd, int sockfd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_add(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void cws_epoll_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_del(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void cws_fd_set_nonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		CWS_LOG_ERROR("fcntl(): %s", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
}

int cws_server_accept_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size) {
	const int client_fd = accept(sockfd, (struct sockaddr *)their_sa, theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			CWS_LOG_ERROR("accept(): %s", strerror(errno));
		}
		return -1;
	}

	return client_fd;
}

void cws_server_close_client(int epfd, int client_fd, cws_hashmap *hashmap) {
	cws_epoll_del(epfd, client_fd);
	cws_hm_remove(hashmap, &client_fd);
}

SSL_CTX *cws_ssl_create_context() {
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = TLS_server_method();
	ctx = SSL_CTX_new(method);

	if (!ctx) {
		CWS_LOG_ERROR("SSL_CTX_new()");
		return NULL;
	}

	return ctx;
}

bool cws_ssl_configure(SSL_CTX *context, cws_config *config) {
	int ret;

	ret = SSL_CTX_use_certificate_file(context, config->cert, SSL_FILETYPE_PEM);
	if (ret <= 0) {
		CWS_LOG_ERROR("SSL_CTX_use_certificate_file()");

		return false;
	}

	ret = SSL_CTX_use_PrivateKey_file(context, config->key, SSL_FILETYPE_PEM);
	if (ret <= 0) {
		CWS_LOG_ERROR("SSL_CTX_use_PrivateKey_file()");

		return false;
	}

	return true;
}
