#ifndef CWS_SERVER_H
#define CWS_SERVER_H

#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>

#include "myclib/hashmap/myhashmap.h"
#include "utils/config.h"

/* Clients max queue */
#define CWS_SERVER_BACKLOG 128

/* Size of the epoll_event array */
#define CWS_SERVER_EPOLL_MAXEVENTS 64

#define CWS_SERVER_EPOLL_TIMEOUT 3000

#define CWS_SERVER_MAX_REQUEST_SIZE (16 * 1024) /* 16KB */

/* Main server loop */
extern volatile sig_atomic_t cws_server_run;

typedef enum cws_server_ret_t {
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

cws_server_ret cws_server_start(cws_config *config);
cws_server_ret cws_server_loop(int server_fd, cws_config *config);
int cws_server_handle_new_client(int server_fd, mcl_hashmap *clients);
int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size);
cws_server_ret cws_fd_set_nonblocking(int sockfd);

#endif
