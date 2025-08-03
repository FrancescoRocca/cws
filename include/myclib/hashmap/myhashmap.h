#ifndef MYCLIB_HASHMAP_H
#define MYCLIB_HASHMAP_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#define MYCLIB_HASHMAP_SIZE 1024 /**< Number of buckets in the hash map */

/**
 * @brief A single bucket in the hash map
 *
 * Each bucket can hold one key-value pair and points to the next bucket
 * in case of hash collisions (separate chaining).
 */
typedef struct mcl_bucket_t {
	void *key;				   /**< Pointer to the key */
	void *value;			   /**< Pointer to the value */
	struct mcl_bucket_t *next; /**< Pointer to the next bucket in case of collision */
} mcl_bucket;

/**
 * @brief Function pointer type for a hash function
 *
 * @param[in] key Pointer to the key to hash
 * @return The computed hash as an unsigned integer
 */
typedef unsigned int mcl_hash_fn(const void *key);

/**
 * @brief Function pointer type for a key comparison function
 *
 * @param[in] key_a Pointer to the first key
 * @param[in] key_b Pointer to the second key
 * @return true if the keys are considered equal, false otherwise
 */
typedef bool mcl_equal_fn(const void *key_a, const void *key_b);

/**
 * @brief Function pointer type for freeing a key
 *
 * @param[in] key Pointer to the key to free
 */
typedef void mcl_free_key_fn(void *key);

/**
 * @brief Function pointer type for freeing a value
 *
 * @param[in] value Pointer to the value to free
 */
typedef void mcl_free_value_fn(void *value);

/**
 * @brief Main structure representing the hash map
 *
 * Contains function pointers for hash computation, key comparison,
 * and memory management, along with the bucket array.
 */
typedef struct mcl_hashmap_t {
	mcl_hash_fn *hash_fn;				 /**< Hash function */
	mcl_equal_fn *equal_fn;				 /**< Equality comparison function */
	mcl_free_key_fn *free_key_fn;		 /**< Key deallocation function (optional) */
	mcl_free_value_fn *free_value_fn;	 /**< Value deallocation function (optional) */
	size_t key_size;					 /**< Size in bytes of the key */
	size_t value_size;					 /**< Size in bytes of the value */
	mcl_bucket map[MYCLIB_HASHMAP_SIZE]; /**< Array of bucket chains */
	pthread_mutex_t *locks;				 /**< Mutex array */
	size_t num_locks;					 /**< Number of mutex */
} mcl_hashmap;

/**
 * @brief Initialize a new hash map with user-defined behavior functions
 *
 * Creates a new hash map and initializes it with the provided function pointers.
 * The free functions can be NULL if no automatic memory management is needed.
 * Keys and values will be copied into the hashmap using memcpy with the specified sizes.
 *
 * @param[in] hash_fn Function used to hash keys (required)
 * @param[in] equal_fn Function used to compare keys (required)
 * @param[in] free_key_fn Function used to free keys (optional, can be NULL)
 * @param[in] free_value_fn Function used to free values (optional, can be NULL)
 * @param[in] key_size Size in bytes of each key to be stored
 * @param[in] value_size Size in bytes of each value to be stored
 * @return A pointer to the newly initialized hash map, or NULL on failure
 */
mcl_hashmap *mcl_hm_init(mcl_hash_fn *hash_fn, mcl_equal_fn *equal_fn, mcl_free_key_fn *free_key_fn, mcl_free_value_fn *free_value_fn, size_t key_size,
						 size_t value_size);

/**
 * @brief Free all resources used by the hash map
 *
 * Iterates through all buckets, frees keys and values using the provided
 * free functions (if not NULL), and deallocates the hash map structure.
 *
 * @param[in] hashmap Pointer to the hash map to free
 */
void mcl_hm_free(mcl_hashmap *hashmap);

/**
 * @brief Free a bucket returned by mcl_hm_get()
 *
 * @param[in] bucket Pointer to the bucket to free
 */
void mcl_hm_free_bucket(mcl_bucket *bucket);

/**
 * @brief Insert or update a key-value pair in the hash map
 *
 * If the key already exists, the old value is freed (if free_value_fn is provided)
 * and replaced with the new value. If the key doesn't exist, a new entry is created.
 * Both key and value are copied into the hashmap using memcpy.
 *
 * @param[in] hashmap Pointer to the hash map
 * @param[in] key Pointer to the key to insert (will be copied, must not be NULL)
 * @param[in] value Pointer to the value to insert (will be copied, must not be NULL)
 * @return true if the operation succeeded, false on failure (NULL hashmap/key/value or memory allocation failure)
 */
bool mcl_hm_set(mcl_hashmap *hashmap, void *key, void *value);

/**
 * @brief Retrieve a bucket by key
 *
 * Searches for the given key in the hash map and returns the bucket containing it.
 * The caller can then access both the key and value from the returned bucket.
 *
 * @param[in] hashmap Pointer to the hash map
 * @param[in] key Pointer to the key to search for
 * @return Pointer to the copy of the bucket, to avoid race conditions, or NULL if not found or on invalid input
 */
mcl_bucket *mcl_hm_get(mcl_hashmap *hashmap, void *key);

/**
 * @brief Remove a key-value pair from the hash map
 *
 * Searches for the given key and removes it from the hash map. Both the key
 * and value are freed using the provided free functions (if not NULL).
 *
 * @param[in] hashmap Pointer to the hash map
 * @param[in] key Pointer to the key to remove
 * @return true if the key was found and removed, false if not found or on invalid input
 */
bool mcl_hm_remove(mcl_hashmap *hashmap, void *key);

#endif /* MYCLIB_HASHMAP_H */
