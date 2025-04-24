#ifndef CWS_HASHMAP_H
#define CWS_HASHMAP_H

#include <stdbool.h>
#include <sys/socket.h>

/* Each process on Linux can have a maximum of 1024 open file descriptors */
#define CWS_HASHMAP_MAX_CLIENTS 1024

/**
 * @brief Hash map struct
 *
 */
typedef struct cws_bucket_t {
	int sockfd;					 /**< Client socket descriptor */
	struct sockaddr_storage sas; /**< Associated socket address */
	struct cws_bucket_t *next;	 /**< Next node in case of collision */
	struct cws_bucket_t *prev;	 /**< Previous node in case of collision */
} cws_bucket;

/**
 * @brief Calculates the hash code of a given file descriptor
 *
 * @param[in] sockfd The file descriptor
 * @return Returns the hash code
 */
int cws_hm_hash(int sockfd);

/**
 * @brief Initializes the hash map
 *
 * @param[out] bucket The hash map uninitialized
 */
void cws_hm_init(cws_bucket *bucket);

/**
 * @brief Inserts a key in the hash map
 *
 * @param[out] bucket The hash map
 * @param[in] sockfd The file descriptor (value)
 * @param[in] sas The sockaddr (value)
 */
void cws_hm_push(cws_bucket *bucket, int sockfd, struct sockaddr_storage *sas);

/**
 * @brief Removes a key from the hash map
 *
 * @param[out] bucket The hash map
 * @param[in] sockfd The key
 */
void cws_hm_remove(cws_bucket *bucket, int sockfd);

/**
 * @brief Searches for a key in the hash map
 *
 * @param[in] bucket The hash map
 * @param[in] sockfd The file descriptor (key)
 * @return Returns NULL or the key pointer
 */
cws_bucket *cws_hm_lookup(cws_bucket *bucket, int sockfd);

/**
 * @brief Cleans the hash map
 *
 * @param[out] bucket The hash map
 */
void cws_hm_free(cws_bucket *bucket);

/**
 * @brief Checks if a file descriptor is in the bucket array (not linked list)
 *
 * @param[in] bucket
 * @param[in] sockfd
 * @return true If the file descriptor is in the bucket array
 * @return false If the file descriptor is not in the bucket array (check with hm_lookup())
 */
bool cws_hm_is_in_bucket_array(cws_bucket *bucket, int sockfd);

/**
 * @brief This function will add a key even if it exists (use hm_push() instead)
 *
 * @param bucket
 * @param sockfd
 * @param sas
 */
void cws_hm_insert(cws_bucket *bucket, int sockfd, struct sockaddr_storage *sas);

#endif