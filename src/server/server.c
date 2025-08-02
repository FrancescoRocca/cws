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

volatile sig_atomic_t cws_server_run = 1;

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

int cws_server_start(cws_config *config) {
	struct addrinfo hints;
	struct addrinfo *res;

	cws_server_setup_hints(&hints, sizeof hints, config->hostname);

	int status = getaddrinfo(config->hostname, config->port, &hints, &res);
	if (status != 0) {
		CWS_LOG_ERROR("getaddrinfo() error: %s", gai_strerror(status));
		return -1;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		CWS_LOG_ERROR("socket(): %s", strerror(errno));
		return -1;
	}

	const int opt = 1;
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		CWS_LOG_ERROR("setsockopt(): %s", strerror(errno));
		return -1;
	}

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		CWS_LOG_ERROR("bind(): %s", strerror(errno));
		return -1;
	}

	status = listen(sockfd, CWS_SERVER_BACKLOG);
	if (status != 0) {
		CWS_LOG_ERROR("listen(): %s", strerror(errno));
		return -1;
	}

	int ret = cws_server_loop(sockfd, config);
	CWS_LOG_DEBUG("cws_server_loop ret: %d", ret);

	freeaddrinfo(res);
	close(sockfd);

	return 0;
}

int cws_server_loop(int sockfd, cws_config *config) {
	mcl_hashmap *clients = mcl_hm_init(my_int_hash_fn, my_int_equal_fn, my_int_free_key_fn, my_str_free_fn);
	if (!clients) {
		return -1;
	}

	int ret = 0;
	int epfd = epoll_create1(0);

	ret = cws_fd_set_nonblocking(sockfd);
	if (ret < 0) {
		mcl_hm_free(clients);
		close(epfd);

		return -1;
	}

	ret = cws_epoll_add(epfd, sockfd, EPOLLIN | EPOLLET);
	if (ret < 0) {
		mcl_hm_free(clients);
		close(epfd);

		return -1;
	}

	struct epoll_event *revents = malloc(CWS_SERVER_EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	if (!revents) {
		mcl_hm_free(clients);
		close(epfd);

		return -1;
	}

	while (cws_server_run) {
		int nfds = epoll_wait(epfd, revents, CWS_SERVER_EPOLL_MAXEVENTS, CWS_SERVER_EPOLL_TIMEOUT);

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == sockfd) {
				ret = cws_server_handle_new_client(sockfd, epfd, clients);
				if (ret < 0) {
					CWS_LOG_DEBUG("handle new client error");
				}
			} else {
				ret = cws_server_handle_client_data(revents[i].data.fd, epfd, clients, config);
				if (ret < 0) {
					CWS_LOG_DEBUG("handle client data error");
				}
			}
		}
	}

	/* Clean up everything */
	free(revents);
	close(epfd);
	mcl_hm_free(clients);

	return 0;
}

int cws_server_handle_new_client(int sockfd, int epfd, mcl_hashmap *clients) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;
	char ip[INET_ADDRSTRLEN];

	int client_fd = cws_server_accept_client(sockfd, &their_sa, &theirsa_size);
	if (client_fd < 0) {
		return -1;
	}

	cws_utils_get_client_ip(&their_sa, ip);
	CWS_LOG_INFO("Client (%s) (fd: %d) connected", ip, client_fd);

	cws_fd_set_nonblocking(client_fd);
	cws_epoll_add(epfd, client_fd, EPOLLIN);

	int *key = malloc(sizeof(int));
	*key = client_fd;
	struct sockaddr_storage *value = malloc(sizeof(struct sockaddr_storage));
	*value = their_sa;

	mcl_hm_set(clients, key, value);

	return 0;
}

int cws_server_handle_client_data(int client_fd, int epfd, mcl_hashmap *clients, cws_config *config) {
	char data[4096] = {0};
	char ip[INET_ADDRSTRLEN] = {0};

	/* Incoming data */
	const ssize_t bytes_read = recv(client_fd, data, sizeof(data), 0);

	/* Retrieve client ip */
	int *client_fd_key = malloc(sizeof(int));
	*client_fd_key = client_fd;
	mcl_bucket *client = mcl_hm_get(clients, client_fd_key);
	free(client_fd_key);

	if (!client) {
		CWS_LOG_ERROR("Client fd %d not found in hashmap", client_fd);
		cws_epoll_del(epfd, client_fd);
		close(client_fd);

		return -1;
	}

	struct sockaddr_storage client_sas = *(struct sockaddr_storage *)client->value;
	cws_utils_get_client_ip(&client_sas, ip);

	if (bytes_read == 0) {
		/* Client disconnected */
		CWS_LOG_INFO("Client (%s) disconnected", ip);
		cws_server_close_client(epfd, client_fd, clients);

		return -1;
	}

	if (bytes_read < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			/* Error during read, handle it (close client) */
			CWS_LOG_INFO("Client (%s) disconnected (error)", ip);
			cws_server_close_client(epfd, client_fd, clients);
		}

		return -1;
	}

	/* Parse HTTP request */
	cws_http *request = cws_http_parse(data, client_fd, config);

	if (request == NULL) {
		CWS_LOG_INFO("Client (%s) disconnected (request NULL)", ip);
		cws_server_close_client(epfd, client_fd, clients);

		return -1;
	}

	cws_http_send_resource(request);
	cws_http_free(request);

	return 0;
}

int cws_epoll_add(int epfd, int sockfd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_add(): %s", strerror(errno));
		return -1;
	}

	return 0;
}

int cws_epoll_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_del(): %s", strerror(errno));
		return -1;
	}

	return 0;
}

int cws_fd_set_nonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		CWS_LOG_ERROR("fcntl(): %s", strerror(errno));
		return -1;
	}

	return 0;
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

void cws_server_close_client(int epfd, int client_fd, mcl_hashmap *hashmap) {
	cws_epoll_del(epfd, client_fd);

	int *key_to_find = malloc(sizeof(int));
	*key_to_find = client_fd;
	mcl_hm_remove(hashmap, key_to_find);
	free(key_to_find);
}
