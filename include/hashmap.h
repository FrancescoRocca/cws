#ifndef __HASHMAP_C__
#define __HASHMAP_C__

#include <string.h>
#include <sys/socket.h>

#define HASHMAP_MAX_CLIENTS 10000

struct hashmap {
	int sockfd;
	struct sockaddr_storage sas;
};

int hash(int sockfd);
void hm_init(struct hashmap *map);
void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas);
struct hashmap *hm_lookup(struct hashmap *map, int sockfd);

#endif