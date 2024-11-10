#ifndef __HASHMAP_C__
#define __HASHMAP_C__

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* Each process on Linux can have a maximum of 1024 open file descriptors */
#define HASHMAP_MAX_CLIENTS 1024

struct hashmap {
	int sockfd;
	struct sockaddr_storage sas;
	struct hashmap *next;
};

/* Calculate the hash code of a file descriptor */
int hash(int sockfd);

/* Initialize the hash map */
void hm_init(struct hashmap *map);

/* Insert a new key in the hash map */
void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas);

/* Search for a key in the hash map */
struct hashmap *hm_lookup(struct hashmap *map, int sockfd);

/* Clean up the hash map */
void hm_free(struct hashmap *map);

#endif