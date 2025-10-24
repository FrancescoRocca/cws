#include "utils/utils.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

cws_server_ret cws_fd_set_nonblocking(int sockfd) {
	const int status = fcntl(sockfd, F_SETFL, O_NONBLOCK);

	if (status == -1) {
		return CWS_SERVER_FD_NONBLOCKING_ERROR;
	}

	return CWS_SERVER_OK;
}

unsigned int my_str_hash_fn(const void *key) {
	char *key_str = (char *)key;
	size_t key_len = strlen(key_str);

	int total = 0;

	for (size_t i = 0; i < key_len; ++i) {
		total += (int)key_str[i];
	}

	return total % 2069;
}

bool my_str_equal_fn(const void *a, const void *b) {
	if (strcmp((char *)a, (char *)b) == 0) {
		return true;
	}

	return false;
}

void my_str_free_fn(void *value) {
	free(value);
}

unsigned int my_int_hash_fn(const void *key) {
	return *(int *)key;
}

bool my_int_equal_fn(const void *a, const void *b) {
	int ai = *(int *)a;
	int bi = *(int *)b;

	if (ai == bi) {
		return true;
	}

	return false;
}

void my_int_free_key_fn(void *key) {
	int fd = *(int *)key;
	close(fd);
	free(key);
}
