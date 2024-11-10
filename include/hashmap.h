#ifndef __HASHMAP_C__
#define __HASHMAP_C__

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* Each process on Linux can have a maximum of 1024 open file descriptors */
#define HASHMAP_MAX_CLIENTS 1024

typedef struct bucket {
	int sockfd;		     /**< Client socket descriptor */
	struct sockaddr_storage sas; /**< Associated socket address */
	struct bucket *next;	     /**< Next node in case of collision */
	struct bucket *prev;	     /**< Previous node in case of collision */
} bucket_t;

/**
 * @brief Calculates the hash code of a given file descriptor
 *
 * @param sockfd[in] The file descriptor
 * @return int Returns the hash code
 */
int hash(int sockfd);

/**
 * @brief Initializes the hash map
 *
 * @param bucket[out] The hash map uninitialized
 */
void hm_init(bucket_t *bucket);

/**
 * @brief Inserts a key in the hash map
 *
 * @param bucket[out] The hash map
 * @param sockfd[in] The file descriptor (value)
 * @param sas[in] The sockaddr (value)
 */
void hm_push(bucket_t *bucket, int sockfd, struct sockaddr_storage *sas);

/**
 * @brief Removes a key from the hash map
 *
 * @param bucket[out] The hash map
 * @param sockfd[in] The key
 */
void hm_remove(bucket_t *bucket, int sockfd);

/**
 * @brief Searches for a key in the hash map
 *
 * @param bucket[in] The hash map
 * @param sockfd[in] The file descriptor (key)
 * @return struct hashmap* Returns NULL or the key pointer
 */
bucket_t *hm_lookup(bucket_t *bucket, int sockfd);

/**
 * @brief Cleans the hash map
 *
 * @param bucket[out] The hash map
 */
void hm_free(bucket_t *bucket);

/**
 * @brief Checks if a file descriptor is in the bucket array (not linked list)
 *
 * @param bucket[in]
 * @param sockfd[in]
 * @return true If the file descriptor is in the bucket array
 * @return false If the file descriptor is not in the bucket array (check with hm_lookup())
 */
bool hm_is_in_bucket_array(bucket_t *bucket, int sockfd);

/**
 * @brief This function will add a key even if it exists (use hm_push() instead)
 *
 * @param bucket
 * @param sockfd
 * @param sas
 */
void hm_insert(bucket_t *bucket, int sockfd, struct sockaddr_storage *sas);

#endif