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

#define PORT 3030
// how many pending connections the queue will hold
#define BACKLOG 10

int start_server(const char *hostname, const char *service);

#endif
