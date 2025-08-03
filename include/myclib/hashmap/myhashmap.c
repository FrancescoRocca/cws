#include "myhashmap.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t mcl_get_mutex(mcl_hashmap *hashmap, size_t hash) { return hash % hashmap->num_locks; }

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

mcl_hashmap *mcl_hm_init(mcl_hash_fn *hash_fn, mcl_equal_fn *equal_fn, mcl_free_key_fn *free_key_fn, mcl_free_value_fn *free_value_fn, size_t key_size,
						 size_t value_size) {
	mcl_hashmap *hashmap = malloc(sizeof(mcl_hashmap));
	if (hashmap == NULL) {
		return NULL;
	}

	hashmap->hash_fn = hash_fn;
	hashmap->equal_fn = equal_fn;
	hashmap->free_key_fn = free_key_fn;
	hashmap->free_value_fn = free_value_fn;
	hashmap->key_size = key_size;
	hashmap->value_size = value_size;

	hashmap->num_locks = 64;
	hashmap->locks = malloc(sizeof(pthread_mutex_t) * hashmap->num_locks);
	if (hashmap->locks == NULL) {
		free(hashmap);

		return NULL;
	}

	int ret;
	for (size_t i = 0; i < hashmap->num_locks; ++i) {
		ret = pthread_mutex_init(&(hashmap->locks[i]), NULL);
		if (ret != 0) {
			/* Mutex failed */
			for (size_t j = 0; j < i; ++j) {
				pthread_mutex_destroy(&(hashmap->locks[j]));
			}

			free(hashmap->locks);
			free(hashmap);
		}
	}

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

	/* Free the mutex */
	for (size_t i = 0; i < hashmap->num_locks; ++i) {
		pthread_mutex_destroy(&(hashmap->locks[i]));
	}
	free(hashmap->locks);

	/* Free the hash map structure itself */
	free(hashmap);
}

void mcl_hm_free_bucket(mcl_bucket *bucket) {
	if (bucket == NULL) {
		return;
	}

	free(bucket->key);
	free(bucket->value);
	free(bucket);
}

bool mcl_hm_set(mcl_hashmap *hashmap, void *key, void *value) {
	if (hashmap == NULL || key == NULL || value == NULL) {
		return false;
	}

	size_t mutex_id = mcl_get_mutex(hashmap, hashmap->hash_fn(key));
	pthread_mutex_t *mutex = &(hashmap->locks[mutex_id]);
	pthread_mutex_lock(mutex);

	mcl_bucket *prev;
	mcl_bucket *existing = mcl_find_bucket(hashmap, key, &prev);

	if (existing != NULL) {
		/* Key exists, update value */
		if (hashmap->free_value_fn != NULL && existing->value != NULL) {
			hashmap->free_value_fn(existing->value);
		}

		existing->value = malloc(hashmap->value_size);
		if (existing->value == NULL) {
			pthread_mutex_unlock(mutex);

			return false;
		}

		memcpy(existing->value, value, hashmap->value_size);
		pthread_mutex_unlock(mutex);

		return true;
	}

	/* Key doesn't exist, need to insert new bucket */
	size_t index = mcl_get_bucket_index(hashmap, key);
	mcl_bucket *bucket = &hashmap->map[index];

	if (bucket->key == NULL) {
		/* First bucket is empty, use it */
		bucket->key = malloc(hashmap->key_size);
		if (bucket->key == NULL) {
			pthread_mutex_unlock(mutex);

			return false;
		}

		bucket->value = malloc(hashmap->value_size);
		if (bucket->value == NULL) {
			free(bucket->key);
			bucket->key = NULL;
			pthread_mutex_unlock(mutex);

			return false;
		}

		memcpy(bucket->key, key, hashmap->key_size);
		memcpy(bucket->value, value, hashmap->value_size);
		bucket->next = NULL;
		pthread_mutex_unlock(mutex);

		return true;
	}

	/* Create new bucket and insert at head of collision chain */
	mcl_bucket *new_bucket = malloc(sizeof(mcl_bucket));
	if (new_bucket == NULL) {
		pthread_mutex_unlock(mutex);

		return false;
	}

	new_bucket->key = malloc(hashmap->key_size);
	if (new_bucket->key == NULL) {
		free(new_bucket);
		pthread_mutex_unlock(mutex);

		return false;
	}

	new_bucket->value = malloc(hashmap->value_size);
	if (new_bucket->value == NULL) {
		free(new_bucket->key);
		free(new_bucket);
		pthread_mutex_unlock(mutex);

		return false;
	}

	memcpy(new_bucket->key, key, hashmap->key_size);
	memcpy(new_bucket->value, value, hashmap->value_size);
	new_bucket->next = bucket->next;
	bucket->next = new_bucket;
	pthread_mutex_unlock(mutex);

	return true;
}

static mcl_bucket *mcl_get_bucket_copy(mcl_bucket *from, size_t key_size, size_t value_size) {
	mcl_bucket *copy = malloc(sizeof(mcl_bucket));
	if (copy == NULL) {
		return NULL;
	}
	memcpy(copy, from, sizeof(mcl_bucket));

	copy->key = malloc(key_size);
	if (copy->key == NULL) {
		free(copy);

		return NULL;
	}
	memcpy(copy->key, from->key, key_size);

	copy->value = malloc(value_size);
	if (copy->value == NULL) {
		free(copy->key);
		free(copy);

		return NULL;
	}
	memcpy(copy->value, from->value, value_size);

	return copy;
}

mcl_bucket *mcl_hm_get(mcl_hashmap *hashmap, void *key) {
	if (hashmap == NULL || key == NULL) {
		return NULL;
	}

	size_t mutex_id = mcl_get_mutex(hashmap, hashmap->hash_fn(key));
	pthread_mutex_t *mutex = &(hashmap->locks[mutex_id]);
	pthread_mutex_lock(mutex);

	mcl_bucket *prev;
	mcl_bucket *found = mcl_find_bucket(hashmap, key, &prev);

	if (found) {
		mcl_bucket *copy = mcl_get_bucket_copy(found, hashmap->key_size, hashmap->value_size);

		pthread_mutex_unlock(mutex);

		return copy;
	}

	pthread_mutex_unlock(mutex);

	return NULL;
}

bool mcl_hm_remove(mcl_hashmap *hashmap, void *key) {
	if (hashmap == NULL || key == NULL) {
		return false;
	}

	size_t mutex_id = mcl_get_mutex(hashmap, hashmap->hash_fn(key));
	pthread_mutex_t *mutex = &(hashmap->locks[mutex_id]);
	pthread_mutex_lock(mutex);

	mcl_bucket *prev;
	mcl_bucket *to_remove = mcl_find_bucket(hashmap, key, &prev);

	if (to_remove == NULL) {
		pthread_mutex_unlock(mutex);

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

	pthread_mutex_unlock(mutex);

	return true;
}
