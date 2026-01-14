#include "core/socket.h"

#include <errno.h>
#include <sys/socket.h>

size_t cws_socket_read(int sockfd, string_s *str) {
	char tmp[4096] = {0};

	size_t n = recv(sockfd, tmp, sizeof tmp, 0);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			/* No data available right now */
			return 0;
		}

		return -1;
	}

	if (n == 0) {
		/* Connection closed */
		return -1;
	}

	tmp[n] = '\0';
	string_append(str, tmp);
	return n;
}

size_t cws_socket_send(int sockfd, char *buffer, size_t len, int flags) {
	size_t total_sent = 0;
	size_t n;

	while (total_sent < len) {
		n = send(sockfd, buffer + total_sent, len - total_sent, flags);

		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* Buffer full, return bytes sent */
				break;
			}

			return -1;
		}

		if (n == 0) {
			break;
		}

		total_sent += n;
	}

	return total_sent;
}
