#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_client_connection(const char *hostname, const char *port);

#endif
