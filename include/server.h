#ifndef __SERVER_H__
#define __SERVER_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 3030
#define BACKLOG 10
#define EPOLL_MAXEVENTS 10
#define EPOLL_TIMEOUT -1

int start_server(const char *hostname, const char *service);
void setup_hints(struct addrinfo *hints, size_t len, const char *hostname);
void handle_clients(int sockfd);
void epoll_ctl_add(int epfd, int sockfd, uint32_t events);
void setnonblocking(int sockfd);
void handle_new_client(int sockfd);

#endif
