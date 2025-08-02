#include "utils/utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

char *cws_strip(char *str) {
	char *end;

	while (isspace((int)*str)) str++;

	if (*str == 0) return str;

	end = str + strlen(str) - 1;
	while (end > str && isspace((int)*end)) end--;
	*(end + 1) = '\0';

	return str;
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

void my_str_free_fn(void *value) { free(value); }

unsigned int my_int_hash_fn(const void *key) { return *(int *)key; }

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
