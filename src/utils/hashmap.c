#include "utils/hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cws_hashmap *cws_hm_init(cws_hash_fn *hash_fn, cws_equal_fn *equal_fn, cws_free_key_fn *free_key_fn, cws_free_value_fn *free_value_fn) {
	/* Allocate hash map struct */
	cws_hashmap *hashmap = (cws_hashmap *)malloc(sizeof(cws_hashmap));

	if (hashmap == NULL) {
		return NULL;
	}

	/* Populate hash map with given parameters */
	hashmap->hash_fn = hash_fn;
	hashmap->equal_fn = equal_fn;
	hashmap->free_key_fn = free_key_fn;
	hashmap->free_value_fn = free_value_fn;
	/* Clear cws_bucket map */
	memset(hashmap->map, 0, sizeof(hashmap->map));

	return hashmap;
}

void cws_hm_free(cws_hashmap *hashmap) {
	if (hashmap == NULL) {
		return;
	}

	for (size_t i = 0; i < CWS_HASHMAP_SIZE; ++i) {
		cws_bucket *bucket = &hashmap->map[i];
		if (bucket->key != NULL) {
			if (hashmap->free_key_fn != NULL) {
				hashmap->free_key_fn(bucket->key);
			}
			if (hashmap->free_value_fn != NULL) {
				hashmap->free_value_fn(bucket->value);
			}
		}

		bucket = bucket->next;
		while (bucket) {
			if (hashmap->free_key_fn != NULL) {
				hashmap->free_key_fn(bucket->key);
			}
			if (hashmap->free_value_fn != NULL) {
				hashmap->free_value_fn(bucket->value);
			}
			cws_bucket *next = bucket->next;
			free(bucket);
			bucket = next;
		}
	}

	free(hashmap);
}

bool cws_hm_set(cws_hashmap *hashmap, void *key, void *value) {
	/* Get hash index */
	int index = hashmap->hash_fn(key) % CWS_HASHMAP_SIZE;
	cws_bucket *bucket = &hashmap->map[index];

	/* Check if the key at index is empty */
	if (bucket->key == NULL) {
		/* Bucket is empty */
		/* Set the key and value */
		bucket->key = key;
		bucket->value = value;
		bucket->next = NULL;

		return true;
	}

	/* Check if bucket is already set */
	if (hashmap->equal_fn(bucket->key, key)) {
		/* Same key, free value and update it */
		if (hashmap->free_value_fn != NULL) {
			hashmap->free_value_fn(bucket->value);
		}
		bucket->value = value;

		return true;
	}

	/* Key not found, iterate through the linked list */
	cws_bucket *next = bucket->next;
	while (next) {
		if (hashmap->equal_fn(next->key, key)) {
			/* Same key, free value and update it */
			if (hashmap->free_value_fn != NULL) {
				hashmap->free_value_fn(next->value);
			}
			next->value = value;
			return true;
		}
		next = next->next;
	}

	/* Append the new key/value to the head of the linked list */
	next = (cws_bucket *)malloc(sizeof(cws_bucket));
	if (next == NULL) {
		return false;
	}
	next->key = key;
	next->value = value;
	next->next = bucket->next;
	bucket->next = next;

	return true;
}

cws_bucket *cws_hm_get(cws_hashmap *hashmap, void *key) {
	/* Return if key is null */
	if (key == NULL) {
		return NULL;
	}

	int index = hashmap->hash_fn(key);
	cws_bucket *bucket = &hashmap->map[index];

	/* Key is not in the hash map */
	if (bucket == NULL || bucket->key == NULL) {
		return NULL;
	}

	/* Iterate through the linked list */
	while (bucket) {
		if (hashmap->equal_fn(bucket->key, key)) {
			return bucket;
		}
		bucket = bucket->next;
	}

	/* Key not found */
	return NULL;
}
