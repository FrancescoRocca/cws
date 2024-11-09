#include "hashmap.h"

int hash(int sockfd) { return sockfd % HASHMAP_MAX_CLIENTS; }

void hm_init(struct hashmap *map) {
	memset(map, 0, sizeof(struct hashmap) * HASHMAP_MAX_CLIENTS);

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		map[i].sockfd = -1;
	}
}

void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas) {
	int index = hash(sockfd);
	map[index].sockfd = sockfd;
	map[index].sas = *sas;

    // I should check the collisions
}

struct hashmap *hm_lookup(struct hashmap *map, int sockfd) {
	int index = hash(sockfd);
	return &map[index];
}
