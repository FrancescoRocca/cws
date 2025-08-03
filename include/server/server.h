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
} cws_server_ret;

/**
 * @brief Runs the server
 *
 * @param[in] config The server's config
 */
cws_server_ret cws_server_start(cws_config *config);

/**
 * @brief Main server loop
 *
 * @param[in,out] sockfd Socket of the commincation endpoint
 */
cws_server_ret cws_server_loop(int server_fd, cws_config *config);

/**
 * @brief Adds a file descriptor to the interest list
 *
 * @param[in] epfd epoll file descriptor
 * @param[in] sockfd The file descriptor to watch
 * @param[in] events The events to follow
 */
cws_server_ret cws_epoll_add(int epfd, int sockfd, uint32_t events);

/**
 * @brief Removes a file descriptor from the interest list
 *
 * @param[in] epfd epoll file descriptor
 * @param[in] sockfd The file descriptor to remove
 */
cws_server_ret cws_epoll_del(int epfd, int sockfd);

/**
 * @brief Makes a file descriptor non-blocking
 *
 * @param[in] sockfd The file descriptor to make non-blocking
 */
cws_server_ret cws_fd_set_nonblocking(int sockfd);

/**
 * @brief Handles the new client
 *
 * @param[in] sockfd Server's file descriptor
 * @param[out] their_sa Populates the struct with client's information
 * @param[in] theirsa_size Size of the struct
 */
int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size);

/**
 * @brief Disconnect a client
 *
 * @param[in] epfd Epoll file descriptor
 * @param[in] client_fd Client file descriptor
 * @param[in] hashmap Clients hash map
 */
void cws_server_close_client(int epfd, int client_fd, mcl_hashmap *hashmap);

int cws_server_handle_new_client(int server_fd, int epfd, mcl_hashmap *clients);
void *cws_server_handle_client_data(void *arg);

#endif
