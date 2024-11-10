#ifndef __HASHMAP_C__
#define __HASHMAP_C__

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* Each process on Linux can have a maximum of 1024 open file descriptors */
#define HASHMAP_MAX_CLIENTS 1024

struct hashmap {
	int sockfd;
	struct sockaddr_storage sas;
	struct hashmap *next;
};

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
 * @param map[out] The hash map uninitialized
 */
void hm_init(struct hashmap *map);

/**
 * @brief Inserts a key in the hash map
 *
 * @param map[out] The hash map
 * @param sockfd[in] The file descriptor (value)
 * @param sas[in] The sockaddr (value)
 */
void hm_push(struct hashmap *map, int sockfd, struct sockaddr_storage *sas);

/**
 * @brief Searches for a key in the hash map
 *
 * @param map[in] The hash map
 * @param sockfd[in] The file descriptor (key)
 * @return struct hashmap* Returns NULL or the key pointer
 */
struct hashmap *hm_lookup(struct hashmap *map, int sockfd);

/**
 * @brief Cleans the hash map
 *
 * @param map[out] The hash map
 */
void hm_free(struct hashmap *map);

void hm_insert(struct hashmap *map, int sockfd, struct sockaddr_storage *sas);

#endif