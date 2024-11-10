#include "utils/hashmap.h"

int hash(int sockfd) { return sockfd % HASHMAP_MAX_CLIENTS; }

void hm_init(bucket_t *bucket) {
	/* Initialize everything to 0 for the struct, then -1 for fd and next to NULL */
	memset(bucket, 0, sizeof(bucket_t) * HASHMAP_MAX_CLIENTS);

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		bucket[i].sockfd = -1;
		bucket[i].next = NULL;
		bucket[i].prev = NULL;
	}
}

void hm_insert(bucket_t *bucket, int sockfd, struct sockaddr_storage *sas) {
	int index = hash(sockfd);

	if (bucket[index].sockfd == -1) {
		/* Current slot is empty */
		bucket[index].sockfd = sockfd;
		bucket[index].sas = *sas;
	} else {
		/* Append the new key to the head (not the first element because it belongs to the array) of the linked
		 * list */
		bucket_t *p = &bucket[index];
		bucket_t *new = malloc(sizeof(bucket_t));
		new->sockfd = sockfd;
		new->sas = *sas;
		new->next = p->next;
		new->prev = p;
		p->next = new;
	}
}

bucket_t *hm_lookup(bucket_t *bucket, int sockfd) {
	int index = hash(sockfd);

	if (bucket[index].sockfd != sockfd) {
		bucket_t *p;
		for (p = bucket[index].next; p != NULL && p->sockfd != sockfd; p = p->next);
		return p;
	} else {
		return &bucket[index];
	}

	return NULL;
}

void hm_push(bucket_t *bucket, int sockfd, struct sockaddr_storage *sas) {
	if (hm_lookup(bucket, sockfd) == NULL) {
		hm_insert(bucket, sockfd, sas);
	}
}

void hm_remove(bucket_t *bucket, int sockfd) {
	if (hm_is_in_bucket_array(bucket, sockfd)) {
		/* Instead of doing this I could copy the memory of the next node into the head */
		int index = hash(sockfd);
		bucket[index].sockfd = -1;

		return;
	}

	/* Key not in the bucket array, let's search in the linked list */
	bucket_t *p = hm_lookup(bucket, sockfd);
	if (p == NULL) return;
	p->prev->next = p->next;
	if (p->next != NULL) {
		/* If there's a next node update the previous node */
		p->next->prev = p->prev;
	}
	free(p);
}

void hm_free(bucket_t *bucket) {
	bucket_t *p, *next;

	for (size_t i = 0; i < HASHMAP_MAX_CLIENTS; ++i) {
		if (bucket[i].next != NULL) {
			/* Free the malloc */
			p = bucket[i].next;
			next = p->next;
			do {
				free(p);
				p = next;
				next = p != NULL ? p->next : NULL;
			} while (p != NULL);
		}
	}
}

bool hm_is_in_bucket_array(bucket_t *bucket, int sockfd) {
	int index = hash(sockfd);
	if (bucket[index].sockfd == sockfd) return true;

	return false;
}
