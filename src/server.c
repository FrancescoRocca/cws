#include "server.h"
#include "colors.h"

int start_server(const char *hostname, const char *service) {
	struct addrinfo hints;
	struct addrinfo *res;
	struct sockaddr_storage their_sa; // incoming clients
	socklen_t theirsa_size = sizeof their_sa;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;	 // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;	 // fill in IP for me

	int status = getaddrinfo(hostname, service, &hints, &res);
	if (status != 0) {
		fprintf(stderr,
			RED BOLD "[server] getaddrinfo() error: %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	// socket file descriptor
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

	char *msg = "Hello there!";
	int msg_len = strlen(msg);
	int newfd = accept(sockfd, (struct sockaddr *)&their_sa, &theirsa_size);
	int bytes_sent = send(newfd, msg, msg_len, 0);
	fprintf(stdout, BLUE "[server] Sent %d bytes\n" RESET, bytes_sent);

	freeaddrinfo(res);

	return 0;
}
