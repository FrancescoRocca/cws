#include "core/socket.h"

#include <errno.h>
#include <poll.h>
#include <sys/socket.h>

#include "internal/common.h"

int cws_socket_read(int sockfd, string_s *str) {
	char tmp[4096] = {0};
	int total = 0;

	for (;;) {
		/* NUL terminator */
		ssize_t n = recv(sockfd, tmp, sizeof tmp - 1, 0);

		/* We have some data */
		if (n > 0) {
			tmp[n] = '\0';
			string_append(str, tmp);
			total += n;

			/* Cap the total buffered request size */
			if (string_len(str) > MAX_REQUEST_SIZE) {
				return -1;
			}
			continue;
		}

		/* Client closed */
		if (n == 0) {
			return total > 0 ? total : 0;
		}

		if (errno == EINTR) {
			continue;
		}

		/* No more data available */
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			if (total > 0) {
				return total;
			}
			return -2;
		}

		/* Something happened */
		return -1;
	}
}

ssize_t cws_socket_send(int sockfd, const char *buffer, size_t len, int flags) {
	size_t total_sent = 0;

	while (total_sent < len) {
		ssize_t n = send(sockfd, buffer + total_sent, len - total_sent, flags);

		if (n > 0) {
			total_sent += (size_t)n;
			continue;
		}

		if (n == 0) {
			/* Peer closed the connection */
			return -1;
		}

		if (errno == EINTR) {
			continue;
		}

		/* Socket buffer full: wait until writable, then retry */
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			struct pollfd pfd = {.fd = sockfd, .events = POLLOUT};
			int pr = poll(&pfd, 1, SOCKET_SEND_POLL_TIMEOUT_MS);
			if (pr <= 0) {
				return -1;
			}
			continue;
		}

		return -1;
	}

	return (ssize_t)total_sent;
}
