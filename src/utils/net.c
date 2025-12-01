#include "utils/net.h"
#include "utils/error.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

volatile sig_atomic_t cws_server_run = 1;

static void cws_utils_convert_ip(int family, void *addr, char *ip, size_t ip_len) {
	inet_ntop(family, addr, ip, ip_len);
}

void cws_utils_get_client_ip(struct sockaddr_storage *sa, char *ip) {
	if (sa->ss_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		cws_utils_convert_ip(AF_INET, &sin->sin_addr, ip, INET_ADDRSTRLEN);
	} else if (sa->ss_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		cws_utils_convert_ip(AF_INET6, &sin6->sin6_addr, ip, INET6_ADDRSTRLEN);
	}
}

cws_return cws_fd_set_nonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		return CWS_FD_NONBLOCKING_ERROR;
	}

	return CWS_OK;
}
