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
#include "utils/hashmap.h"
#include "utils/utils.h"

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

	const int opt = 1;
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] setsockopt(): %s\n" RESET, strerror(errno));
		exit(EXIT_FAILURE);
	}

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] bind(): %s\n" RESET, strerror(errno));
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
	/* IPv4 or IPv6 */
	hints->ai_family = AF_UNSPEC;
	/* TCP */
	hints->ai_socktype = SOCK_STREAM;
	if (hostname == NULL) {
		/* Fill in IP for me */
		hints->ai_flags = AI_PASSIVE;
	}
}

void handle_clients(int sockfd) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;

	bucket_t clients[HASHMAP_MAX_CLIENTS];
	hm_init(clients);

	int epfd = epoll_create1(0);
	setnonblocking(sockfd);
	epoll_ctl_add(epfd, sockfd, EPOLLIN | EPOLLET);

	struct epoll_event *revents = malloc(EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	int client_fd;

	while (1) {
		int nfds = epoll_wait(epfd, revents, EPOLL_MAXEVENTS, EPOLL_TIMEOUT);

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == sockfd) {
				/* New client */
				char ip[INET_ADDRSTRLEN];
				client_fd = handle_new_client(sockfd, &their_sa, &theirsa_size);
				get_client_ip(&their_sa, ip);
				fprintf(stdout, BLUE "[server] Client (%s) connected\n" RESET, ip);

				setnonblocking(client_fd);
				epoll_ctl_add(epfd, client_fd, EPOLLIN);
				hm_push(clients, client_fd, &their_sa);
			} else {
				char data[4096] = {0};
				/* Incoming data */
				client_fd = revents[i].data.fd;
				const ssize_t bytes_read = recv(client_fd, data, sizeof data, 0);
				char ip[INET_ADDRSTRLEN];
				bucket_t *client = hm_lookup(clients, client_fd);
				get_client_ip(&client->sas, ip);

				if (bytes_read == 0) {
					/* Client disconnected */
					fprintf(stdout, BLUE "[server] Client (%s) disconnected\n" RESET, ip);
					close_client(epfd, client_fd, clients);
					continue;
				}
				if (bytes_read < 0) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						/* Error during read, handle it (close client) */
						epoll_ctl_del(epfd, client_fd);
						close(client_fd);
					}
					continue;
				}

				data[bytes_read] = '\0';

				/* Parse HTTP request */
				http_t *request = http_parse(data, client_fd);

				if (request == NULL) {
					close_client(epfd, client_fd, clients);
					http_free(request);
					continue;
				}

				http_send_response(request);
				fprintf(stdout, BLUE "[server] Client (%s) disconnected\n" RESET, ip);
				close_client(epfd, client_fd, clients);
				http_free(request);

				/* Clear str */
				memset(data, 0, sizeof data);
			}
		}
	}

	/* Clean up everything */
	/* TODO: fix endless loop using cli args */
	free(revents);
	close(epfd);
	close_fds(clients);
	hm_free(clients);
}

void epoll_ctl_add(int epfd, int sockfd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] epoll_ctl_add(): %s\n" RESET, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void epoll_ctl_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	if (status != 0) {
		fprintf(stdout, RED BOLD "[server] epoll_ctl_del(): %s\n" RESET, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void setnonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		fprintf(stderr, RED BOLD "[server] fcntl(): %s\n" RESET, gai_strerror(status));
		exit(EXIT_FAILURE);
	}
}

int handle_new_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size) {
	const int client_fd = accept(sockfd, (struct sockaddr *)their_sa, theirsa_size);

	if (client_fd == -1) {
		if (errno != EWOULDBLOCK) {
			fprintf(stderr, RED BOLD "[server] accept(): %s\n" RESET, strerror(errno));
		}
		return -1;
	}

	return client_fd;
}

void close_fds(bucket_t *bucket) {
	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		close(bucket[i].sockfd);
		if (bucket[i].next != NULL) {
			/* Close the fds */
			bucket_t *p = bucket[i].next;
			bucket_t *next = p->next;
			do {
				close(p->sockfd);
				p = next;
				next = p != NULL ? p->next : NULL;
			} while (p != NULL);
		}
	}
}

void close_client(int epfd, int client_fd, bucket_t *clients) {
	epoll_ctl_del(epfd, client_fd);
	hm_remove(clients, client_fd);
	close(client_fd);
}