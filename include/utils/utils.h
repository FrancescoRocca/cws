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

/**
 * @brief Remove whitespaces from a string
 *
 * @param[in] str The string pointer
 * @return Returns the new string without whitespaces
 */
char *cws_strip(char *str);

/* Functions used for hash maps */
int my_str_hash_fn(void *key);
bool my_str_equal_fn(void *a, void *b);
void my_str_free_fn(void *value);

int my_int_hash_fn(void *key);
bool my_int_equal_fn(void *a, void *b);
void my_int_free_fn(void *value);

#endif
