#ifndef __HASHMAP_C__
#define __HASHMAP_C__

#include <sys/socket.h>

#define HASHMAP_MAX_ITEMS 10000

struct hashmap {
	int sockfd;
	struct sockaddr_storage sas;
};

int hash(int sockfd);
void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas);

#endif