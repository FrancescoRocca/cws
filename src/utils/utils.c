#include "utils/utils.h"

#include "utils/colors.h"

void print_ips(const char *hostname, const char *port) {
	struct addrinfo ai;
	struct addrinfo *res;

	memset(&ai, 0, sizeof ai);

	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(hostname, port, &ai, &res);
	if (status < 0) {
		fprintf(stderr, RED "getaddrinfo(): %s\n" RESET, gai_strerror(status));
		exit(1);
	}

	char ipv4[INET_ADDRSTRLEN];
	char ipv6[INET6_ADDRSTRLEN];

	for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
		if (p->ai_family == AF_INET) {
			struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;
			inet_ntop(AF_INET, &sin->sin_addr, ipv4, INET_ADDRSTRLEN);
			fprintf(stdout, BLUE "%s\n" RESET, ipv4);
		} else if (p->ai_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)p->ai_addr;
			inet_ntop(AF_INET6, &sin6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
			fprintf(stdout, BLUE "%s\n" RESET, ipv6);
		}
	}

	freeaddrinfo(res);
}

void get_client_ip(struct sockaddr_storage *sa, char *ip) {
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;

	inet_ntop(AF_INET, &sin->sin_addr, ip, INET_ADDRSTRLEN);
}