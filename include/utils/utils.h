#ifndef CWS_UTILS_H
#define CWS_UTILS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

/**
 * @brief Prints each IP address associated with a host
 *
 * @param hostname[in] Hostname
 * @param port[in] Port
 */
void print_ips(const char *hostname, const char *port);

/**
 * @brief Retrieves the client ip from the sockaddr_storage and put it in the ip str
 *
 * @param sa[in] The sockaddr_storage of the client
 * @param ip[out] The IP of the client
 */
void get_client_ip(struct sockaddr_storage *sa, char *ip);

#endif
