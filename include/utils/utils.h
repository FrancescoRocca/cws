#ifndef CWS_UTILS_H
#define CWS_UTILS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>

/**
 * @brief Prints each IP address associated with a host
 *
 * @param[in] hostname Hostname
 * @param[in] port Port
 */
void cws_utils_print_ips(const char *hostname, const char *port);

/**
 * @brief Retrieves the client ip from the sockaddr_storage and put it in the ip str
 *
 * @param[in] sa The sockaddr_storage of the client
 * @param[out] ip The IP of the client
 */
void cws_utils_get_client_ip(struct sockaddr_storage *sa, char *ip);

/* TODO: add docs */
/* Functions used for hash maps */
int my_hash_fn(void *key);
bool my_equal_fn(void *a, void *b);
void my_free_value_fn(void *value);

#endif
