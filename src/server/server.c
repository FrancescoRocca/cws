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
#include "utils/colors.h"
#include "utils/utils.h"

volatile sig_atomic_t cws_server_run = 1;

void cws_server_setup_hints(struct addrinfo *hints, const char *hostname) {
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

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		CWS_LOG_ERROR("socket(): %s", strerror(errno));
		return CWS_SERVER_SOCKET_ERROR;
	}

	const int opt = 1;
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		CWS_LOG_ERROR("setsockopt(): %s", strerror(errno));
		return CWS_SERVER_SETSOCKOPT_ERROR;
	}

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		CWS_LOG_ERROR("bind(): %s", strerror(errno));
		return CWS_SERVER_BIND_ERROR;
	}

	status = listen(sockfd, CWS_SERVER_BACKLOG);
	if (status != 0) {
		CWS_LOG_ERROR("listen(): %s", strerror(errno));
		return CWS_SERVER_LISTEN_ERROR;
	}

	cws_server_ret ret = cws_server_loop(sockfd, config);
	CWS_LOG_DEBUG("cws_server_loop ret: %d", ret);

	freeaddrinfo(res);
	close(sockfd);

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_loop(int sockfd, cws_config *config) {
	mcl_hashmap *clients = mcl_hm_init(my_int_hash_fn, my_int_equal_fn, my_int_free_key_fn, my_str_free_fn, sizeof(int), sizeof(struct sockaddr_storage));
	if (!clients) {
		return CWS_SERVER_HASHMAP_INIT;
	}

	cws_server_ret ret;
	int epfd = epoll_create1(0);

	ret = cws_fd_set_nonblocking(sockfd);
	if (ret != CWS_SERVER_OK) {
		mcl_hm_free(clients);
		close(epfd);

		return ret;
	}

	ret = cws_epoll_add(epfd, sockfd, EPOLLIN | EPOLLET);
	if (ret != CWS_SERVER_OK) {
		mcl_hm_free(clients);
		close(epfd);

		return ret;
	}

	struct epoll_event *revents = malloc(CWS_SERVER_EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	if (!revents) {
		mcl_hm_free(clients);
		close(epfd);

		return CWS_SERVER_MALLOC_ERROR;
	}

	while (cws_server_run) {
		int nfds = epoll_wait(epfd, revents, CWS_SERVER_EPOLL_MAXEVENTS, CWS_SERVER_EPOLL_TIMEOUT);

		if (nfds == 0) {
			CWS_LOG_INFO("epoll timeout, continue...");
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == sockfd) {
				ret = cws_server_handle_new_client(sockfd, epfd, clients);
				if (ret != CWS_SERVER_OK) {
					CWS_LOG_DEBUG("Handle new client: %d", ret);
				}
			} else {
				ret = cws_server_handle_client_data(revents[i].data.fd, epfd, clients, config);
				if (ret != CWS_SERVER_OK) {
					CWS_LOG_DEBUG("Handle client data: %d", ret);
				}
			}
		}
	}

	/* Clean up everything */
	free(revents);
	close(epfd);
	mcl_hm_free(clients);

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_handle_new_client(int sockfd, int epfd, mcl_hashmap *clients) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;
	char ip[INET_ADDRSTRLEN];

	int client_fd = cws_server_accept_client(sockfd, &their_sa, &theirsa_size);
	if (client_fd < 0) {
		return CWS_SERVER_FD_ERROR;
	}

	cws_utils_get_client_ip(&their_sa, ip);
	CWS_LOG_INFO("Client (%s) (fd: %d) connected", ip, client_fd);

	cws_fd_set_nonblocking(client_fd);
	cws_epoll_add(epfd, client_fd, EPOLLIN);
	mcl_hm_set(clients, &client_fd, &their_sa);

	return CWS_SERVER_OK;
}

cws_server_ret cws_server_handle_client_data(int client_fd, int epfd, mcl_hashmap *clients, cws_config *config) {
	char tmp_data[4096];
	memset(tmp_data, 0, sizeof(tmp_data));
	char ip[INET_ADDRSTRLEN] = {0};
	mcl_string *data = mcl_string_new("", 4096);

	/* Incoming data */
	ssize_t total_bytes = 0;
	ssize_t bytes_read;
	while ((bytes_read = recv(client_fd, tmp_data, sizeof(tmp_data), 0)) > 0) {
		total_bytes += bytes_read;
		if (total_bytes > CWS_SERVER_MAX_REQUEST_SIZE) {
			mcl_string_free(data);
			cws_server_close_client(epfd, client_fd, clients);

			return CWS_SERVER_REQUEST_TOO_LARGE;
		}
		mcl_string_append(data, tmp_data);
	}

	if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		/* Error during read, handle it (close client) */
		CWS_LOG_ERROR("recv(): %s", strerror(errno));
		mcl_string_free(data);
		cws_server_close_client(epfd, client_fd, clients);

		return CWS_SERVER_CLIENT_DISCONNECTED_ERROR;
	}

	/* Retrieve client ip */
	mcl_bucket *client = mcl_hm_get(clients, &client_fd);
	if (!client) {
		CWS_LOG_ERROR("Client fd %d not found in hashmap", client_fd);
		cws_epoll_del(epfd, client_fd);
		close(client_fd);

		return CWS_SERVER_CLIENT_NOT_FOUND;
	}
	struct sockaddr_storage *client_sas = (struct sockaddr_storage *)client->value;
	cws_utils_get_client_ip(client_sas, ip);

	if (bytes_read == 0) {
		/* Client disconnected */
		CWS_LOG_INFO("Client (%s) disconnected", ip);
		mcl_string_free(data);
		cws_server_close_client(epfd, client_fd, clients);

		return CWS_SERVER_CLIENT_DISCONNECTED;
	}

	/* Parse HTTP request */
	cws_http *request = cws_http_parse(data, client_fd, config);
	mcl_string_free(data);

	if (request == NULL) {
		CWS_LOG_INFO("Client (%s) disconnected (request NULL)", ip);
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
	mcl_hm_remove(hashmap, &client_fd);
}
