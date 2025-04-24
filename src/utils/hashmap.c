#include "utils/hashmap.h"

#include <stdlib.h>
#include <string.h>

int cws_hm_hash(int sockfd) { return sockfd % CWS_HASHMAP_MAX_CLIENTS; }

void cws_hm_init(cws_bucket *bucket) {
	/* Initialize everything to 0 for the struct, then -1 for fd and next to NULL */
	memset(bucket, 0, sizeof(cws_bucket) * CWS_HASHMAP_MAX_CLIENTS);

	for (size_t i = 0; i < CWS_HASHMAP_MAX_CLIENTS; ++i) {
		bucket[i].sockfd = -1;
		bucket[i].next = NULL;
		bucket[i].prev = NULL;
	}
}

void cws_hm_insert(cws_bucket *bucket, int sockfd, struct sockaddr_storage *sas) {
	int index = cws_hm_hash(sockfd);

	if (bucket[index].sockfd == -1) {
		/* Current slot is empty */
		bucket[index].sockfd = sockfd;
		bucket[index].sas = *sas;
	} else {
		/* Append the new key to the head (not the first element because it belongs to the array) of the linked
		 * list */
		cws_bucket *p = &bucket[index];
		cws_bucket *new = malloc(sizeof(cws_bucket));
		new->sockfd = sockfd;
		new->sas = *sas;
		new->next = p->next;
		new->prev = p;
		p->next = new;
	}
}

cws_bucket *cws_hm_lookup(cws_bucket *bucket, int sockfd) {
	int index = cws_hm_hash(sockfd);

	if (bucket[index].sockfd != sockfd) {
		cws_bucket *p;
		for (p = bucket[index].next; p != NULL && p->sockfd != sockfd; p = p->next);
		return p;
	} else {
		return &bucket[index];
	}

	return NULL;
}

void cws_hm_push(cws_bucket *bucket, int sockfd, struct sockaddr_storage *sas) {
	if (cws_hm_lookup(bucket, sockfd) == NULL) {
		cws_hm_insert(bucket, sockfd, sas);
	}
}

void cws_hm_remove(cws_bucket *bucket, int sockfd) {
	if (cws_hm_is_in_bucket_array(bucket, sockfd)) {
		/* Instead of doing this I could copy the memory of the next node into the head */
		int index = cws_hm_hash(sockfd);
		bucket[index].sockfd = -1;

		return;
	}

	/* Key not in the bucket array, let's search in the linked list */
	cws_bucket *p = cws_hm_lookup(bucket, sockfd);
	if (p == NULL) return;
	p->prev->next = p->next;
	if (p->next != NULL) {
		/* If there's a next node update the previous node */
		p->next->prev = p->prev;
	}
	free(p);
}

void cws_hm_free(cws_bucket *bucket) {
	cws_bucket *p, *next;

	for (size_t i = 0; i < CWS_HASHMAP_MAX_CLIENTS; ++i) {
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

bool cws_hm_is_in_bucket_array(cws_bucket *bucket, int sockfd) {
	int index = cws_hm_hash(sockfd);
	if (bucket[index].sockfd == sockfd) return true;

	return false;
}
