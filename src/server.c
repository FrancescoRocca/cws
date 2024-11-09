#include "server.h"

#include "colors.h"
#include "hashmap.h"
#include "utils.h"

int start_server(const char *hostname, const char *service) {
	struct addrinfo hints;
	struct addrinfo *res;

	setup_hints(&hints, sizeof hints, hostname);

	int status = getaddrinfo(hostname, service, &hints, &res);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] getaddrinfo() error: %s\n" RESET, gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	fprintf(stdout, YELLOW "[server] sockfd: %d\n" RESET, sockfd);

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] bind(): %s\n" RESET, gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	status = listen(sockfd, BACKLOG);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] listen(): %s\n" RESET, gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	handle_clients(sockfd);

	freeaddrinfo(res);
	close(sockfd);

	return 0;
}

void setup_hints(struct addrinfo *hints, size_t len, const char *hostname) {
	memset(hints, 0, len);
	hints->ai_family = AF_UNSPEC;	   // IPv4 or IPv6
	hints->ai_socktype = SOCK_STREAM;  // TCP
	if (hostname == NULL) {
		hints->ai_flags = AI_PASSIVE;  // fill in IP for me
	}
}

void handle_clients(int sockfd) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;

	struct hashmap clients[HASHMAP_MAX_CLIENTS];
	hm_init(clients);

	int epfd = epoll_create1(0);
	setnonblocking(sockfd);
	epoll_ctl_add(epfd, sockfd, EPOLLIN | EPOLLET);

	struct epoll_event *revents = malloc(EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	int nfds;

	char *msg = "Hello there!";
	size_t msg_len = strlen(msg);
	char data[4096];
	int client_fd;
	int run = 1;

	while (run) {
		nfds = epoll_wait(epfd, revents, EPOLL_MAXEVENTS, EPOLL_TIMEOUT);

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == sockfd) {
				// new client
				client_fd = handle_new_client(sockfd, &their_sa, &theirsa_size);

				setnonblocking(client_fd);
				epoll_ctl_add(epfd, client_fd, EPOLLIN);
				hm_insert(clients, client_fd, &their_sa);

				int bytes_sent = send(client_fd, msg, msg_len, 0);
				fprintf(stdout, "[server] Sent %d bytes\n", bytes_sent);
			} else {
				// incoming data
				client_fd = revents[i].data.fd;
				int bytes_read = recv(client_fd, data, sizeof data, 0);

				if (bytes_read == 0) {
					// client disconnected
					char ip[INET_ADDRSTRLEN];
					struct hashmap *client = hm_lookup(clients, client_fd);
					get_client_ip(&client->sas, ip);

					fprintf(stdout, BLUE "[server] Client (%s) disconnected\n" RESET, ip);
					epoll_ctl_del(epfd, client_fd);
					close(client_fd);
					continue;
				}

				data[strcspn(data, "\n")] = '\0';
				fprintf(stdout, "[server] Bytes read (%d): %s\n", bytes_read, data);
				if (strcmp(data, "stop") == 0) {
					fprintf(stdout, GREEN BOLD "[server] Stopping...\n" RESET);
					run = 0;
					break;
				}
			}
		}
	}

	free(revents);
	close(epfd);

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		close(clients[i].sockfd);
	}
}

void epoll_ctl_add(int epfd, int sockfd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = sockfd;
	int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] epoll_ctl_add(): %s\n" RESET, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void epoll_ctl_del(int epfd, int sockfd) {
	int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
	if (status != 0) {
		fprintf(stdout, RED BOLD "[server] epoll_ctl_del(): %s\n" RESET, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void setnonblocking(int sockfd) {
	int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if (status == -1) {
		fprintf(stderr, RED BOLD "[server] fcntl(): %s\n" RESET, gai_strerror(status));
		exit(EXIT_FAILURE);
	}
}

int handle_new_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size) {
	int client_fd = accept(sockfd, (struct sockaddr *)their_sa, theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			fprintf(stderr, RED BOLD "[server] accept(): %s\n" RESET, strerror(errno));
		}
		return -1;
	}

	return client_fd;
}
