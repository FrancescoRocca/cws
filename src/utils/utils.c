#include "utils/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/colors.h"

void cws_utils_print_ips(const char *hostname, const char *port) {
	struct addrinfo ai;
	struct addrinfo *res;

	memset(&ai, 0, sizeof ai);

	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(hostname, port, &ai, &res);
	if (status < 0) {
		CWS_LOG_ERROR("getaddrinfo(): %s", gai_strerror(status));
		exit(1);
	}

	char ipv4[INET_ADDRSTRLEN];
	char ipv6[INET6_ADDRSTRLEN];

	for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
		if (p->ai_family == AF_INET) {
			struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;
			inet_ntop(AF_INET, &sin->sin_addr, ipv4, INET_ADDRSTRLEN);
			CWS_LOG_INFO("%s", ipv4);
		} else if (p->ai_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)p->ai_addr;
			inet_ntop(AF_INET6, &sin6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
			CWS_LOG_INFO("%s", ipv6);
		}
	}

	freeaddrinfo(res);
}

void cws_utils_get_client_ip(struct sockaddr_storage *sa, char *ip) {
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;

	inet_ntop(AF_INET, &sin->sin_addr, ip, INET_ADDRSTRLEN);
}