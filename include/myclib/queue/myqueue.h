#ifndef MYCLIB_QUEUE_H
#define MYCLIB_QUEUE_H

#include <stddef.h>

/**
 * @brief A generic circular queue (ring buffer).
 */
typedef struct mcl_queue_t {
	size_t front;	  /**< Index of the first element (read position). */
	size_t rear;	  /**< Index where the next element will be written (write position). */
	size_t size;	  /**< Current number of elements in the queue. */
	size_t capacity;  /**< Maximum number of elements the queue can hold. */
	size_t elem_size; /**< Size in bytes of each element in the queue. */
	void *buffer;	  /**< Pointer to the memory buffer that stores the elements. */
} mcl_queue;

/**
 * @brief Create and initialize a new queue.
 *
 * @param queue_size Number of elements the queue can hold.
 * @param elem_size Size (in bytes) of each element in the queue.
 * @return Pointer to the new queue, or NULL if allocation fails.
 */
mcl_queue *mcl_queue_init(size_t queue_size, size_t elem_size);

/**
 * @brief Add an element to the queue.
 *
 * @param queue Pointer to the queue.
 * @param elem Pointer to the element to insert.
 * @return 0 on success, -1 if the queue is full.
 */
int mcl_queue_push(mcl_queue *queue, const void *elem);

/**
 * @brief Remove an element from the queue.
 *
 * @param queue Pointer to the queue.
 * @param out_elem Pointer where the removed element will be copied.
 * @return 0 on success, -1 if the queue is empty.
 */
int mcl_queue_pop(mcl_queue *queue, void *out_elem);

/**
 * @brief Get a pointer to the front element of the queue (oldest one).
 *
 * @param queue Pointer to the queue.
 * @return Pointer to the front element, or NULL if the queue is empty.
 */
void *mcl_queue_get_front(mcl_queue *queue);

/**
 * @brief Get a pointer to the rear element of the queue (most recently added).
 *
 * @param queue Pointer to the queue.
 * @return Pointer to the rear element, or NULL if the queue is empty.
 */
void *mcl_queue_get_rear(mcl_queue *queue);

/**
 * @brief Free all memory used by the queue.
 *
 * @param queue Pointer to the queue to free.
 */
void mcl_queue_free(mcl_queue *queue);

#endif
