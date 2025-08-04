#include "server/server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

#include "server/worker.h"
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
	/* Ignore SIGPIPE so a write/sendfile to a closed peer doesn't terminate the process */
	signal(SIGPIPE, SIG_IGN);

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

cws_server_ret cws_server_loop(int server_fd, cws_config *config) {
	mcl_hashmap *clients = mcl_hm_init(my_int_hash_fn, my_int_equal_fn, my_int_free_key_fn, my_str_free_fn, sizeof(int), sizeof(char) * INET_ADDRSTRLEN);
	if (clients == NULL) {
		return CWS_SERVER_HASHMAP_INIT;
	}

	size_t workers_num = 4;
	size_t workers_index = 0;
	cws_worker **workers = cws_worker_init(workers_num, clients, config);
	if (workers == NULL) {
		mcl_hm_free(clients);
		return CWS_SERVER_WORKER_ERROR;
	}

	int client_fd = 0;
	while (cws_server_run) {
		client_fd = cws_server_handle_new_client(server_fd, clients);
		if (client_fd < 0) {
			continue;
		}

		/* Add client to worker */
		int random = 10;
		mcl_hm_set(clients, &client_fd, &random);
		write(workers[workers_index]->pipefd[1], &client_fd, sizeof(int));
		workers_index = (workers_index + 1) % workers_num;
	}

	cws_worker_free(workers, workers_num);
	mcl_hm_free(clients);

	return CWS_SERVER_OK;
}

int cws_server_handle_new_client(int server_fd, mcl_hashmap *clients) {
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

int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size) {
	const int client_fd = accept(server_fd, (struct sockaddr *)their_sa, theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			CWS_LOG_ERROR("accept(): %s", strerror(errno));
		}
	}

	return client_fd;
}

cws_server_ret cws_fd_set_nonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		CWS_LOG_ERROR("fcntl(): %s", strerror(errno));
		return CWS_SERVER_FD_NONBLOCKING_ERROR;
	}

	return CWS_SERVER_OK;
}
