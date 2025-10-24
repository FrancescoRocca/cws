#include "utils/socket.h"

#include <errno.h>
#include <sys/socket.h>

ssize_t cws_read_data(int sockfd, string_s *str) {
	char tmp[4096] = {0};

	ssize_t n = recv(sockfd, tmp, sizeof tmp, 0);
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

ssize_t cws_send_data(int sockfd, char *buffer, int len, int flags) {
	ssize_t total_sent = 0;
	ssize_t n;

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
