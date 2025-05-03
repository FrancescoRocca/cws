#ifndef CWS_HASHMAP_H
#define CWS_HASHMAP_H

#include <stdbool.h>

#define CWS_HASHMAP_SIZE 1024 /**< Number of buckets in the hash map */

/**
 * @brief A single bucket in the hash map.
 */
typedef struct cws_bucket_t {
	void *key;				   /**< Pointer to the key */
	void *value;			   /**< Pointer to the value */
	struct cws_bucket_t *next; /**< Pointer to the next bucket in case of collision */
} cws_bucket;

/**
 * @brief Function pointer type for a hash function
 *
 * @param[in] key Pointer to the key to hash
 * @return The computed hash as an integer
 */
typedef int cws_hash_fn(void *key);

/**
 * @brief Function pointer type for a key comparison function
 *
 * @param[in] key_a Pointer to the first key
 * @param[in] key_b Pointer to the second key
 * @return true if the keys are considered equal, false otherwise
 */
typedef bool cws_equal_fn(void *key_a, void *key_b);

/**
 * @brief Function pointer type for freeing a key
 *
 * @param[in] key Pointer to the key to free
 */
typedef void cws_free_key_fn(void *key);

/**
 * @brief Function pointer type for freeing a value
 *
 * @param[in] value Pointer to the value to free
 */
typedef void cws_free_value_fn(void *value);

/**
 * @brief Main structure representing the hash map
 */
typedef struct cws_hashmap_t {
	cws_hash_fn *hash_fn;			  /**< Hash function */
	cws_equal_fn *equal_fn;			  /**< Equality comparison function */
	cws_free_key_fn *free_key_fn;	  /**< Key deallocation function */
	cws_free_value_fn *free_value_fn; /**< Value deallocation function */

	cws_bucket map[CWS_HASHMAP_SIZE]; /**< Array of bucket chains */
} cws_hashmap;

/**
 * @brief Initializes a new hash map with user-defined behavior
 *
 * @param[in] hash_fn Function used to hash keys
 * @param[in] equal_fn Function used to compare keys
 * @param[in] free_key_fn Function used to free keys
 * @param[in] free_value_fn Function used to free values
 * @return A pointer to the newly initialized hash map
 */
cws_hashmap *cws_hm_init(cws_hash_fn *hash_fn, cws_equal_fn *equal_fn, cws_free_key_fn *free_key_fn, cws_free_value_fn *free_value_fn);

/**
 * @brief Frees all resources used by the hash map
 *
 * @param[in] hashmap Pointer to the hash map to free
 */
void cws_hm_free(cws_hashmap *hashmap);

/**
 * @brief Inserts or updates a key-value pair in the hash map
 *
 * @param[in] hashmap Pointer to the hash map
 * @param[in] key Pointer to the key to insert
 * @param[in] value Pointer to the value to insert
 * @return true if the operation succeeded, false otherwise
 */
bool cws_hm_set(cws_hashmap *hashmap, void *key, void *value);

/**
 * @brief Retrieves a bucket by key
 *
 * @param[in] hashmap Pointer to the hash map
 * @param[in] key Pointer to the key to search for
 * @return Pointer to the found bucket, or NULL if not found
 */
cws_bucket *cws_hm_get(cws_hashmap *hashmap, void *key);

/**
 * @brief Removes a key-value pair from the hash map
 *
 * @param[in] hashmap Pointer to the hash map
 * @param[in] key Pointer to the key to remove, this pointer will be freed so pay attention
 * @return False if the key is not found, otherwise true
 */
bool cws_hm_remove(cws_hashmap *hashmap, void *key);

#endif
