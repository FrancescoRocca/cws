#include "hashmap.h"

int hash(int sockfd) { return sockfd % HASHMAP_MAX_ITEMS; }

void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas) {
	int index = hash(sockfd);
	map[index].sockfd = sockfd;
	map[index].sas = *sas;
}
