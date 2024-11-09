#include "server.h"
#include "colors.h"

int start_server(const char *hostname, const char *service) {
	struct addrinfo hints;
	struct addrinfo *res;

	setup_hints(&hints, sizeof hints, hostname);

	int status = getaddrinfo(hostname, service, &hints, &res);
	if (status != 0) {
		fprintf(stderr,
			RED BOLD "[server] getaddrinfo() error: %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	status = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] bind(): %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	status = listen(sockfd, BACKLOG);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] listen(): %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	handle_clients(sockfd);

	freeaddrinfo(res);
	close(sockfd);

	return 0;
}

void setup_hints(struct addrinfo *hints, size_t len, const char *hostname) {
	memset(hints, 0, len);
	hints->ai_family = AF_UNSPEC;	  // IPv4 or IPv6
	hints->ai_socktype = SOCK_STREAM; // TCP
	if (hostname == NULL) {
		hints->ai_flags = AI_PASSIVE; // fill in IP for me
	}
}

void handle_clients(int sockfd) {
	struct sockaddr_storage their_sa;
	socklen_t theirsa_size = sizeof their_sa;

	int epfd = epoll_create1(0);
	setnonblocking(sockfd);
	epoll_ctl_add(epfd, sockfd, EPOLLIN | EPOLLET);

	struct epoll_event *revents =
	    malloc(EPOLL_MAXEVENTS * sizeof(struct epoll_event));
	int nfds;

	char *msg = "Hello there!";
	size_t msg_len = strlen(msg);

	for (;;) {
		nfds =
		    epoll_wait(epfd, revents, EPOLL_MAXEVENTS, EPOLL_TIMEOUT);

		for (int i = 0; i < nfds; ++i) {
			if (revents[i].data.fd == sockfd) {
				int client_fd =
				    accept(sockfd, (struct sockaddr *)&their_sa,
					   &theirsa_size);

				if (client_fd == -1) {
					if (errno != EWOULDBLOCK) {
						fprintf(stderr,
							RED BOLD "[server] "
								 "accept(): "
								 "%s\n" RESET,
							strerror(errno));
					}
					continue;
				}

				setnonblocking(client_fd);

				struct sockaddr_in *client =
				    (struct sockaddr_in *)&their_sa;
				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &client->sin_addr, client_ip,
					  INET_ADDRSTRLEN);
				fprintf(stdout,
					BLUE "[server] Incoming "
					     "client, ip: %s\n" RESET,
					client_ip);

				int bytes_sent =
				    send(client_fd, msg, msg_len, 0);
				fprintf(stdout,
					BLUE "[server] Sent %d bytes\n" RESET,
					bytes_sent);
				close(client_fd);
			}
		}
	}
}

void epoll_ctl_add(int epfd, int sockfd, uint32_t events) {
	struct epoll_event event;
	event.events = events;
	event.data.fd = sockfd;
	int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[server] epoll_ctl(): %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}
}

void setnonblocking(int sockfd) {
	int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if (status == -1) {
		fprintf(stderr, RED BOLD "[server] fcntl(): %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}
}

void handle_new_client(int sockfd) {
	// handle here new clients
}
