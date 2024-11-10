#include "hashmap.h"

int hash(int sockfd) { return sockfd % HASHMAP_MAX_CLIENTS; }

void hm_init(struct hashmap *map) {
	memset(map, 0, sizeof(struct hashmap) * HASHMAP_MAX_CLIENTS);

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		map[i].sockfd = -1;
		map[i].next = NULL;
	}
}

void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas) {
	int index = hash(sockfd);

	if (map[index].sockfd == -1) {
		map[index].sockfd = sockfd;
		map[index].sas = *sas;
	} else {
		/* Go through the linked list and append the new item */
		struct hashmap *p;
		for (p = &map[index]; p->next != NULL; p = p->next);
		struct hashmap *new = malloc(sizeof(struct hashmap));
		new->sockfd = sockfd;
		new->sas = *sas;
		new->next = NULL;

		p->next = new;
	}
}

struct hashmap *hm_lookup(struct hashmap *map, int sockfd) {
	int index = hash(sockfd);

	if (map[index].sockfd != sockfd) {
		struct hashmap *p;
		for (p = map[index].next; p->sockfd != sockfd; p = p->next);
		return p;
	} else {
		return &map[index];
	}

	return NULL;
}

void hm_free(struct hashmap *map) {}
