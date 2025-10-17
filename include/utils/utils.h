#ifndef CWS_UTILS_H
#define CWS_UTILS_H

#include <stdbool.h>
#include <sys/socket.h>

typedef enum cws_server_ret {
	CWS_SERVER_OK,
	CWS_SERVER_CONFIG,
	CWS_SERVER_FD_ERROR,
	CWS_SERVER_CLIENT_NOT_FOUND,
	CWS_SERVER_CLIENT_DISCONNECTED,
	CWS_SERVER_CLIENT_DISCONNECTED_ERROR,
	CWS_SERVER_HTTP_PARSE_ERROR,
	CWS_SERVER_GETADDRINFO_ERROR,
	CWS_SERVER_SOCKET_ERROR,
	CWS_SERVER_SETSOCKOPT_ERROR,
	CWS_SERVER_BIND_ERROR,
	CWS_SERVER_LISTEN_ERROR,
	CWS_SERVER_EPOLL_ADD_ERROR,
	CWS_SERVER_EPOLL_DEL_ERROR,
	CWS_SERVER_FD_NONBLOCKING_ERROR,
	CWS_SERVER_ACCEPT_CLIENT_ERROR,
	CWS_SERVER_HASHMAP_INIT,
	CWS_SERVER_MALLOC_ERROR,
	CWS_SERVER_REQUEST_TOO_LARGE,
	CWS_SERVER_THREADPOOL_ERROR,
	CWS_SERVER_EPOLL_CREATE_ERROR,
	CWS_SERVER_WORKER_ERROR,
} cws_server_ret;

cws_server_ret cws_fd_set_nonblocking(int sockfd);

void cws_utils_get_client_ip(struct sockaddr_storage *sa, char *ip);

/* Functions used for hash maps */
unsigned int my_str_hash_fn(const void *key);
bool my_str_equal_fn(const void *a, const void *b);
void my_str_free_fn(void *value);

unsigned int my_int_hash_fn(const void *key);
bool my_int_equal_fn(const void *a, const void *b);
void my_int_free_key_fn(void *key);

#endif
