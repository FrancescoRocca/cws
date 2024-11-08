// THIS FILE IS ONLY A TEST FOR THE BASIC STUFF

#include "client.h"
#include "colors.h"

int test_client_connection(const char *hostname, const char *port) {
	struct addrinfo hints;
	struct addrinfo *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	int status = getaddrinfo(hostname, port, &hints, &res);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[client] getaddrinfo(): %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	status = connect(sockfd, res->ai_addr, res->ai_addrlen);
	if (status != 0) {
		fprintf(stderr, RED BOLD "[client] connect(): %s\n" RESET,
			gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	char buf[4096];
	int bytes_read = recv(sockfd, buf, 4096, 0);
	fprintf(stdout, BLUE "[client] Read %d bytes: %s\n" RESET, bytes_read,
		buf);

	freeaddrinfo(res);

	return 0;
}