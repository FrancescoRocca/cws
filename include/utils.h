#ifndef __UTILS_H__
#define __UTILS_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

// print every IPs of a hostname
void print_ips(const char *hostname, const char *port);
void get_client_ip(struct sockaddr_storage *sa, char *ip);

#endif
