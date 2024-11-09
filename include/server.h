#ifndef __SERVER_H__
#define __SERVER_H__

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 3030
#define BACKLOG 10
#define EPOLL_MAXEVENTS 10
#define EPOLL_TIMEOUT -1

int start_server(const char *hostname, const char *service);
void setup_hints(struct addrinfo *hints, size_t len, const char *hostname);
void handle_clients(int sockfd);
void epoll_ctl_add(int epfd, int sockfd, uint32_t events);
void epoll_ctl_del(int epfd, int sockfd);
void setnonblocking(int sockfd);
int handle_new_client(int sockfd, struct sockaddr_storage *their_sa, socklen_t *theirsa_size);

#endif
