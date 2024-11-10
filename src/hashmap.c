#include "hashmap.h"

int hash(int sockfd) { return sockfd % HASHMAP_MAX_CLIENTS; }

void hm_init(struct hashmap *map) {
	/* Initialize everything to 0 for the struct, then -1 for fd and next to NULL */
	memset(map, 0, sizeof(struct hashmap) * HASHMAP_MAX_CLIENTS);

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		map[i].sockfd = -1;
		map[i].next = NULL;
	}
}

/* This function will add a key even if it exists (use hm_push() instead) */
void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas) {
	int index = hash(sockfd);

	if (map[index].sockfd == -1) {
		/* Current slot is empty */
		map[index].sockfd = sockfd;
		map[index].sas = *sas;
		map[index].next = NULL;
	} else {
		/* Append the new key to the head (not the first element because it belongs to the array) of the linked
		 * list */
		struct hashmap *p = &map[index];
		struct hashmap *new = malloc(sizeof(struct hashmap));
		new->sockfd = sockfd;
		new->sas = *sas;
		new->next = p->next;
		p->next = new;
	}
}

struct hashmap *hm_lookup(struct hashmap *map, int sockfd) {
	int index = hash(sockfd);

	if (map[index].sockfd != sockfd) {
		struct hashmap *p;
		for (p = map[index].next; p != NULL && p->sockfd != sockfd; p = p->next);
		return p;
	} else {
		return &map[index];
	}

	return NULL;
}

void hm_push(struct hashmap *map, int sockfd, struct sockaddr_storage *sas) {
	if (hm_lookup(map, sockfd) == NULL) {
		hm_insert(map, sockfd, sas);
	}
}

void hm_free(struct hashmap *map) {
	struct hashmap *p, *next;

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		if (map[i].next != NULL) {
			/* Free the malloc */
			p = map[i].next;
			next = p->next;
			do {
				free(p);
				p = next;
				next = p != NULL ? p->next : NULL;
			} while (p != NULL);
		}
	}
}
