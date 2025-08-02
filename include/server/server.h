#ifndef CWS_SERVER_H
#define CWS_SERVER_H

#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <sys/socket.h>

#include "myclib/hashmap/myhashmap.h"
#include "utils/config.h"

/* Clients max queue */
#define CWS_SERVER_BACKLOG 10

/* Size of the epoll_event array */
#define CWS_SERVER_EPOLL_MAXEVENTS 10

/* Wait forever (epoll_wait()) */
#define CWS_SERVER_EPOLL_TIMEOUT -1

#define CWS_SERVER_MAX_REQUEST_SIZE (64 * 1024) /* 64 KB */

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
} cws_server_ret;

/**
 * @brief Setups hints object
 *
 * @param[out] hints The hints addrinfo
 * @param[in] len The length of hints
 * @param[in] hostname The hostname (could be NULL)
 */
void cws_server_setup_hints(struct addrinfo *hints, size_t len, const char *hostname);

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
cws_server_ret cws_server_loop(int sockfd, cws_config *config);

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
int cws_server_accept_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size);

/**
 * @brief Disconnect a client
 *
 * @param[in] epfd Epoll file descriptor
 * @param[in] client_fd Client file descriptor
 * @param[in] hashmap Clients hash map
 */
void cws_server_close_client(int epfd, int client_fd, mcl_hashmap *hashmap);

cws_server_ret cws_server_handle_new_client(int sockfd, int epfd, mcl_hashmap *clients);
cws_server_ret cws_server_handle_client_data(int client_fd, int epfd, mcl_hashmap *clients, cws_config *config);

#endif
