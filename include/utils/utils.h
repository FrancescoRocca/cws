#ifndef CWS_UTILS_H
#define CWS_UTILS_H

#include <myclib/mysocket.h>
#include <stdbool.h>

void cws_utils_get_client_ip(struct sockaddr_storage *sa, char *ip);

/* Functions used for hash maps */
unsigned int my_str_hash_fn(const void *key);
bool my_str_equal_fn(const void *a, const void *b);
void my_str_free_fn(void *value);

unsigned int my_int_hash_fn(const void *key);
bool my_int_equal_fn(const void *a, const void *b);
void my_int_free_key_fn(void *key);

#endif
