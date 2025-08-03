#include "myqueue.h"

#include <stdlib.h>
#include <string.h>

mcl_queue *mcl_queue_init(size_t queue_size, size_t elem_size) {
	mcl_queue *queue = malloc(sizeof(mcl_queue));
	if (queue == NULL) {
		return NULL;
	}

	queue->buffer = malloc(queue_size * elem_size);
	if (queue->buffer == NULL) {
		free(queue);

		return NULL;
	}

	queue->front = 0;
	queue->rear = 0;
	queue->size = 0;
	queue->capacity = queue_size;
	queue->elem_size = elem_size;

	return queue;
}

int mcl_queue_push(mcl_queue *queue, const void *elem) {
	if (queue->size == queue->capacity) {
		/* Queue full */
		return -1;
	}

	/* Copy the elem in the buffer */
	void *dest = (void *)queue->buffer + (queue->rear * queue->elem_size);
	memcpy(dest, elem, queue->elem_size);

	queue->size++;
	queue->rear = (queue->rear + 1) % queue->capacity;

	return 0;
}

int mcl_queue_pop(mcl_queue *queue, void *out_elem) {
	if (queue->size == 0) {
		/* Queue empty */
		return -1;
	}

	void *src = (void *)queue->buffer + (queue->front * queue->elem_size);
	memcpy(out_elem, src, queue->elem_size);

	queue->front = (queue->front + 1) % queue->capacity;
	queue->size--;

	return 0;
}

void *mcl_queue_get_front(mcl_queue *queue) {
	if (queue->size == 0) {
		return NULL;
	}

	return (void *)queue->buffer + (queue->front * queue->elem_size);
}

void *mcl_queue_get_rear(mcl_queue *queue) {
	if (queue->size == 0) {
		return NULL;
	}

	size_t rear_index;
	if (queue->rear == 0) {
		rear_index = queue->capacity - 1;
	} else {
		rear_index = queue->rear - 1;
	}

	return (void *)queue->buffer + (rear_index * queue->elem_size);
}

void mcl_queue_free(mcl_queue *queue) {
	if (queue == NULL) {
		return;
	}

	free(queue->buffer);
	free(queue);
}
