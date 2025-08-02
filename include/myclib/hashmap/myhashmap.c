#include "myhashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t mcl_get_bucket_index(mcl_hashmap *hashmap, void *key) {
	unsigned int hash = hashmap->hash_fn(key);
	return hash % MYCLIB_HASHMAP_SIZE;
}

static void mcl_free_bucket_content(mcl_hashmap *hashmap, mcl_bucket *bucket) {
	if (bucket == NULL) {
		return;
	}

	/* Free key if free function is provided */
	if (hashmap->free_key_fn != NULL && bucket->key != NULL) {
		hashmap->free_key_fn(bucket->key);
	}

	/* Free value if free function is provided */
	if (hashmap->free_value_fn != NULL && bucket->value != NULL) {
		hashmap->free_value_fn(bucket->value);
	}
}

static mcl_bucket *mcl_find_bucket(mcl_hashmap *hashmap, void *key, mcl_bucket **prev) {
	size_t index = mcl_get_bucket_index(hashmap, key);
	mcl_bucket *bucket = &hashmap->map[index];

	*prev = NULL;

	/* Return NULL if first bucket is empty */
	if (bucket->key == NULL) {
		return NULL;
	}

	/* Search through the collision chain */
	while (bucket != NULL) {
		if (hashmap->equal_fn(bucket->key, key)) {
			return bucket;
		}
		*prev = bucket;
		bucket = bucket->next;
	}

	return NULL;
}

mcl_hashmap *mcl_hm_init(mcl_hash_fn *hash_fn, mcl_equal_fn *equal_fn, mcl_free_key_fn *free_key_fn, mcl_free_value_fn *free_value_fn) {
	mcl_hashmap *hashmap = malloc(sizeof(mcl_hashmap));
	if (hashmap == NULL) {
		return NULL;
	}

	/* Initialize hash map with given parameters */
	hashmap->hash_fn = hash_fn;
	hashmap->equal_fn = equal_fn;
	hashmap->free_key_fn = free_key_fn;
	hashmap->free_value_fn = free_value_fn;

	/* Clear all buckets in the map */
	memset(hashmap->map, 0, sizeof(hashmap->map));

	return hashmap;
}

void mcl_hm_free(mcl_hashmap *hashmap) {
	if (hashmap == NULL) {
		return;
	}

	/* Iterate through all buckets in the hash map */
	for (size_t i = 0; i < MYCLIB_HASHMAP_SIZE; ++i) {
		mcl_bucket *bucket = &hashmap->map[i];

		/* Free the first bucket if it contains data */
		if (bucket->key != NULL) {
			mcl_free_bucket_content(hashmap, bucket);
		}

		/* Free all chained buckets */
		bucket = bucket->next;
		while (bucket != NULL) {
			mcl_bucket *next = bucket->next;
			mcl_free_bucket_content(hashmap, bucket);
			free(bucket);
			bucket = next;
		}
	}

	/* Free the hash map structure itself */
	free(hashmap);
}

bool mcl_hm_set(mcl_hashmap *hashmap, void *key, void *value) {
	if (hashmap == NULL || key == NULL) {
		return false;
	}

	/* Try to find existing bucket */
	mcl_bucket *prev;
	mcl_bucket *existing = mcl_find_bucket(hashmap, key, &prev);

	if (existing != NULL) {
		/* Key exists, update value */
		if (hashmap->free_value_fn != NULL && existing->value != NULL) {
			hashmap->free_value_fn(existing->value);
		}
		existing->value = value;
		return true;
	}

	/* Key doesn't exist, need to insert new bucket */
	size_t index = mcl_get_bucket_index(hashmap, key);
	mcl_bucket *bucket = &hashmap->map[index];

	/* If first bucket is empty, use it */
	if (bucket->key == NULL) {
		bucket->key = key;
		bucket->value = value;
		bucket->next = NULL;
		return true;
	}

	/* Create new bucket and insert at head of collision chain */
	mcl_bucket *new_bucket = malloc(sizeof(mcl_bucket));
	if (new_bucket == NULL) {
		return false;
	}

	new_bucket->key = key;
	new_bucket->value = value;
	new_bucket->next = bucket->next;
	bucket->next = new_bucket;

	return true;
}

mcl_bucket *mcl_hm_get(mcl_hashmap *hashmap, void *key) {
	if (hashmap == NULL || key == NULL) {
		return NULL;
	}

	mcl_bucket *prev;
	return mcl_find_bucket(hashmap, key, &prev);
}

bool mcl_hm_remove(mcl_hashmap *hashmap, void *key) {
	if (hashmap == NULL || key == NULL) {
		return false;
	}

	mcl_bucket *prev;
	mcl_bucket *to_remove = mcl_find_bucket(hashmap, key, &prev);

	if (to_remove == NULL) {
		return false;
	}

	/* Free the content of the bucket */
	mcl_free_bucket_content(hashmap, to_remove);

	/* Handle removal based on position in chain */
	if (prev == NULL) {
		/* Removing first bucket in chain */
		if (to_remove->next != NULL) {
			/* Move next bucket's content to first bucket and free the next bucket */
			mcl_bucket *next_bucket = to_remove->next;
			to_remove->key = next_bucket->key;
			to_remove->value = next_bucket->value;
			to_remove->next = next_bucket->next;
			free(next_bucket);
		} else {
			/* No next bucket, mark first bucket as empty */
			to_remove->key = NULL;
			to_remove->value = NULL;
			to_remove->next = NULL;
		}
	} else {
		/* Removing bucket from middle/end of chain */
		prev->next = to_remove->next;
		free(to_remove);
	}

	return true;
}
