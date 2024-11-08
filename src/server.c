#include "server.h"
#include "colors.h"

int start_server(void) {
	char ipv4[INET_ADDRSTRLEN];
	struct sockaddr_in sa;

	inet_pton(AF_INET, "192.168.0.1", &(sa.sin_addr));
	inet_ntop(AF_INET, &(sa.sin_addr), ipv4, INET_ADDRSTRLEN);

	fprintf(stdout, BLUE "IPv4: %s\n" RESET, ipv4);

	return 0;
}

void get_local_ip(void) {
	char service[] = "3030";
	struct addrinfo hints;
	struct addrinfo *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;	 // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;	 // fill in IP for me

	int status = getaddrinfo(NULL, service, &hints, &res);
	if (status != 0) {
		fprintf(stderr, RED "getaddrinfo() error: %s\n" RESET,
			gai_strerror(status));
		exit(1);
	}

	freeaddrinfo(res);
}
