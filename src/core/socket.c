#include "core/socket.h"

#include <errno.h>
#include <sys/socket.h>

int cws_socket_read(int sockfd, string_s *str) {
	char tmp[4096] = {0};

	for (;;) {
		ssize_t n = recv(sockfd, tmp, sizeof tmp, 0);

		/* We have some data */
		if (n > 0) {
			tmp[n] = '\0';
			string_append(str, tmp);
			return n;
		}

		/* Client closed */
		if (n == 0) {
			return 0;
		}

		if (errno == EINTR) {
			continue;
		}

		/* No data now */
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -2;
		}

		/* Something happened */
		return -1;
	}
}

int cws_socket_send(int sockfd, const char *buffer, size_t len, int flags) {
	size_t total_sent = 0;

	while (total_sent < len) {
		ssize_t n = send(sockfd, buffer + total_sent, len - total_sent, flags);

		if (n > 0) {
			total_sent += (size_t)n;
			continue;
		}

		if (n == 0) {
			break;
		}

		if (errno == EINTR) {
			continue;
		}

		/* Partial write */
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			break;
		}

		return -1;
	}

	return (ssize_t)total_sent;
}
