#ifndef __UTILS_H__
#define __UTILS_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// print every IPs of a hostname
void print_ips(const char *hostname, const char *port);

#endif
