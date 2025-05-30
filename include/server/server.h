#ifndef CWS_SERVER_H
#define CWS_SERVER_H

#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>

#include "utils/config.h"
#include "utils/hashmap.h"

/* Clients max queue */
#define CWS_SERVER_BACKLOG 10

/* Size of the epoll_event array */
#define CWS_SERVER_EPOLL_MAXEVENTS 10

/* Wait forever (epoll_wait()) */
#define CWS_SERVER_EPOLL_TIMEOUT -1

/* Main server loop */
extern volatile bool cws_server_run;

/**
 * @brief Runs the server
 *
 * @param[in] config The server's config
 * @return 0 on success, -1 on error
 */
int cws_server_start(cws_config *config);

/**
 * @brief Setups hints object
 *
 * @param[out] hints The hints addrinfo
 * @param[in] len The length of hints
 * @param[in] hostname The hostname (could be NULL)
 */
void cws_server_setup_hints(struct addrinfo *hints, size_t len, const char *hostname);

/**
 * @brief Main server loop
 *
 * @param[in,out] sockfd Socket of the commincation endpoint
 */
void cws_server_loop(int sockfd, cws_config *config);

/**
 * @brief Adds a file descriptor to the interest list
 *
 * @param[in] epfd epoll file descriptor
 * @param[in] sockfd The file descriptor to watch
 * @param[in] events The events to follow
 */
void cws_epoll_add(int epfd, int sockfd, uint32_t events);

/**
 * @brief Removes a file descriptor from the interest list
 *
 * @param[in] epfd epoll file descriptor
 * @param[in] sockfd The file descriptor to remove
 */
void cws_epoll_del(int epfd, int sockfd);

/**
 * @brief Makes a file descriptor non-blocking
 *
 * @param[in] sockfd The file descriptor to make non-blocking
 */
void cws_fd_set_nonblocking(int sockfd);

/**
 * @brief Handles the new client
 *
 * @param[in] sockfd Server's file descriptor
 * @param[out] their_sa Populates the struct with client's information
 * @param[in] theirsa_size Size of the struct
 * @return Returns -1 on error or the file descriptor on success
 */
int cws_server_accept_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size);

/**
 * @brief Disconnect a client
 *
 * @param[in] epfd Epoll file descriptor
 * @param[in] client_fd Client file descriptor
 * @param[in] hashmap Clients hash map
 */
void cws_server_close_client(int epfd, int client_fd, cws_hashmap *hashmap);

/* Undocumented functions */
SSL_CTX *cws_ssl_create_context();
bool cws_ssl_configure(SSL_CTX *context, cws_config *config);

#endif
