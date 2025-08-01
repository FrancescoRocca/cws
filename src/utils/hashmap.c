#include "utils/hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mcl_hashmap *mcl_hm_init(mcl_hash_fn *hash_fn, mcl_equal_fn *equal_fn, mcl_free_key_fn *free_key_fn, mcl_free_value_fn *free_value_fn) {
	/* Allocate memory for hash map struct */
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
			if (hashmap->free_key_fn != NULL) {
				hashmap->free_key_fn(bucket->key);
			}
			if (hashmap->free_value_fn != NULL) {
				hashmap->free_value_fn(bucket->value);
			}
		}

		/* Free all chained buckets */
		bucket = bucket->next;
		while (bucket != NULL) {
			if (hashmap->free_key_fn != NULL) {
				hashmap->free_key_fn(bucket->key);
			}
			if (hashmap->free_value_fn != NULL) {
				hashmap->free_value_fn(bucket->value);
			}
			mcl_bucket *next = bucket->next;
			free(bucket);
			bucket = next;
		}
	}

	/* Free the hash map structure itself */
	free(hashmap);
}

bool mcl_hm_set(mcl_hashmap *hashmap, void *key, void *value) {
	/* Validate input parameters */
	if (hashmap == NULL || key == NULL) {
		return false;
	}

	/* Calculate hash index for the key */
	int index = hashmap->hash_fn(key) % MYCLIB_HASHMAP_SIZE;
	mcl_bucket *bucket = &hashmap->map[index];

	/* If bucket is empty, insert new key-value pair */
	if (bucket->key == NULL) {
		bucket->key = key;
		bucket->value = value;
		bucket->next = NULL;
		return true;
	}

	/* Check if first bucket has the same key */
	if (hashmap->equal_fn(bucket->key, key)) {
		/* Update existing value, free old value if needed */
		if (hashmap->free_value_fn != NULL && bucket->value != NULL) {
			hashmap->free_value_fn(bucket->value);
		}
		bucket->value = value;
		return true;
	}

	/* Search through the collision chain */
	mcl_bucket *current = bucket->next;
	while (current != NULL) {
		if (hashmap->equal_fn(current->key, key)) {
			/* Update existing value, free old value if needed */
			if (hashmap->free_value_fn != NULL && current->value != NULL) {
				hashmap->free_value_fn(current->value);
			}
			current->value = value;
			return true;
		}
		current = current->next;
	}

	/* Key not found, create new bucket and add to chain */
	mcl_bucket *new_bucket = malloc(sizeof(mcl_bucket));
	if (new_bucket == NULL) {
		return false; /* Memory allocation failed */
	}

	/* Initialize new bucket and insert at head of chain */
	new_bucket->key = key;
	new_bucket->value = value;
	new_bucket->next = bucket->next;
	bucket->next = new_bucket;

	return true;
}

mcl_bucket *mcl_hm_get(mcl_hashmap *hashmap, void *key) {
	/* Validate input parameters */
	if (hashmap == NULL || key == NULL) {
		return NULL;
	}

	/* Calculate hash index for the key */
	int index = hashmap->hash_fn(key) % MYCLIB_HASHMAP_SIZE;
	mcl_bucket *bucket = &hashmap->map[index];

	/* Return NULL if bucket is empty */
	if (bucket->key == NULL) {
		return NULL;
	}

	/* Search through the collision chain */
	while (bucket != NULL) {
		if (hashmap->equal_fn(bucket->key, key)) {
			return bucket; /* Key found */
		}
		bucket = bucket->next;
	}

	/* Key not found */
	return NULL;
}

static void mcl_free_bucket_content(mcl_hashmap *hashmap, mcl_bucket *bucket) {
	/* Free key if free function is provided */
	if (hashmap->free_key_fn != NULL && bucket->key != NULL) {
		hashmap->free_key_fn(bucket->key);
	}

	/* Free value if free function is provided */
	if (hashmap->free_value_fn != NULL && bucket->value != NULL) {
		hashmap->free_value_fn(bucket->value);
	}
}

bool mcl_hm_remove(mcl_hashmap *hashmap, void *key) {
	/* Validate input parameters */
	if (hashmap == NULL || key == NULL) {
		return false;
	}

	/* Calculate hash index for the key */
	int index = hashmap->hash_fn(key) % MYCLIB_HASHMAP_SIZE;
	mcl_bucket *bucket = &hashmap->map[index];

	/* Return false if bucket is empty */
	if (bucket->key == NULL) {
		return false;
	}

	/* Check if first bucket contains the key to remove */
	if (hashmap->equal_fn(bucket->key, key)) {
		/* Free the content of the bucket */
		mcl_free_bucket_content(hashmap, bucket);

		if (bucket->next != NULL) {
			/* Move next bucket's content to first bucket and free the next bucket */
			mcl_bucket *to_free = bucket->next;
			bucket->key = to_free->key;
			bucket->value = to_free->value;
			bucket->next = to_free->next;
			free(to_free);
		} else {
			/* No next bucket, mark first bucket as empty */
			bucket->key = NULL;
			bucket->value = NULL;
			bucket->next = NULL;
		}
		return true;
	}

	/* Search through the collision chain */
	mcl_bucket *prev = bucket;
	mcl_bucket *current = bucket->next;

	while (current != NULL) {
		if (hashmap->equal_fn(current->key, key)) {
			/* Key found, free content and unlink bucket */
			mcl_free_bucket_content(hashmap, current);
			prev->next = current->next;
			free(current);
			return true;
		}
		prev = current;
		current = current->next;
	}

	/* Key not found */
	return false;
}
