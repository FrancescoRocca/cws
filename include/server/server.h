#ifndef CWS_SERVER_H
#define CWS_SERVER_H

#include <netdb.h>
#include <sys/socket.h>

#include "utils/hashmap.h"

/* Clients max queue */
#define BACKLOG 10

/* Size of the epoll_event array */
#define EPOLL_MAXEVENTS 10

/* Wait forever (epoll_wait()) */
#define EPOLL_TIMEOUT -1

/**
 * @brief Runs the server
 *
 * @param hostname[in] The hostname of the server (default localhost, it could be NULL)
 * @param service[in] The service (found in /etc/services) or the port where to run
 * @return 0 on success, -1 on error
 */
int start_server(const char *hostname, const char *service);

/**
 * @brief Setups hints object
 *
 * @param hints[out] The hints addrinfo
 * @param len[in] The length of hints
 * @param hostname[in] The hostname (could be NULL)
 */
void setup_hints(struct addrinfo *hints, size_t len, const char *hostname);

/**
 * @brief Main server loop
 *
 * @param sockfd[in,out] Socket of the commincation endpoint
 */
void handle_clients(int sockfd);

/**
 * @brief Adds a file descriptor to the interest list
 *
 * @param epfd[in] epoll file descriptor
 * @param sockfd[in] The file descriptor to watch
 * @param events[in] The events to follow
 */
void epoll_ctl_add(int epfd, int sockfd, uint32_t events);

/**
 * @brief Removes a file descriptor from the interest list
 *
 * @param epfd[in] epoll file descriptor
 * @param sockfd[in] The file descriptor to remove
 */
void epoll_ctl_del(int epfd, int sockfd);

/**
 * @brief Makes a file descriptor non-blocking
 *
 * @param sockfd[in] The file descriptor to make non-blocking
 */
void setnonblocking(int sockfd);

/**
 * @brief Handles the new client
 *
 * @param sockfd[in] Server's file descriptor
 * @param their_sa[out] Populates the struct with client's information
 * @param theirsa_size[in] Size of the struct
 * @return Returns -1 on error or the file descriptor on success
 */
int handle_new_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size);

/**
 * @brief Closes all the file descriptors opened
 *
 * @param bucket[in] The hash map
 */
void close_fds(bucket_t *bucket);

/**
 * @brief Disconnect a client
 *
 * @param epfd[in] Epoll file descriptor
 * @param client_fd[in] Client file descriptor
 * @param bucket[in] Clients hash map
 */
void close_client(int epfd, int client_fd, bucket_t *bucket);

#endif
