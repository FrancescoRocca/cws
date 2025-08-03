#include "server/threadpool.h"

#include <stdlib.h>
#include <string.h>

cws_threadpool *cws_threadpool_init(size_t threads_num, size_t queue_size) {
	/* Allocate threadpool */
	cws_threadpool *pool = malloc(sizeof(cws_threadpool));
	if (pool == NULL) {
		return NULL;
	}
	memset(pool, 0, sizeof(cws_threadpool));

	int ret;
	ret = pthread_mutex_init(&pool->lock, NULL);
	if (ret != 0) {
		free(pool);

		return NULL;
	}

	ret = pthread_cond_init(&pool->notify, NULL);
	if (ret != 0) {
		pthread_mutex_destroy(&pool->lock);
		free(pool);

		return NULL;
	}

	pool->queue = mcl_queue_init(queue_size, sizeof(cws_thread_task));
	if (pool->queue == NULL) {
		pthread_mutex_destroy(&pool->lock);
		pthread_cond_destroy(&pool->notify);
		free(pool);

		return NULL;
	}

	pool->threads_num = threads_num;
	pool->threads = malloc(sizeof(pthread_t) * threads_num);
	if (pool->threads == NULL) {
		mcl_queue_free(pool->queue);
		pthread_cond_destroy(&pool->notify);
		pthread_mutex_destroy(&pool->lock);
		free(pool);

		return NULL;
	}

	for (size_t i = 0; i < threads_num; ++i) {
		pthread_create(&pool->threads[i], NULL, cws_threadpool_worker, pool);
	}

	return pool;
}

cws_server_ret cws_threadpool_submit(cws_threadpool *pool, cws_thread_task *task) {
	cws_thread_task tt = {
		.function = task->function,
		.arg = task->arg,
	};

	pthread_mutex_lock(&pool->lock);

	int res = mcl_queue_push(pool->queue, &tt);
	if (res == 0) {
		pthread_cond_signal(&pool->notify);
	}

	pthread_mutex_unlock(&pool->lock);

	return res;
}

void *cws_threadpool_worker(void *arg) {
	cws_threadpool *pool = (cws_threadpool *)arg;
	cws_thread_task task = {
		.arg = NULL,
		.function = NULL,
	};
	int ret;

	while (1) {
		/* Loop until notify */
		ret = pthread_mutex_lock(&pool->lock);
		if (ret != 0) {
			continue;
		}

		while (!pool->shutdown && pool->queue->size == 0) {
			pthread_cond_wait(&pool->notify, &pool->lock);
		}

		if (pool->shutdown && pool->queue->size == 0) {
			pthread_mutex_unlock(&pool->lock);
			break;
		}

		mcl_queue_pop(pool->queue, &task);
		pthread_mutex_unlock(&pool->lock);

		task.function(task.arg);
	}

	return NULL;
}

void cws_threadpool_destroy(cws_threadpool *pool) {
	pthread_mutex_lock(&pool->lock);
	pool->shutdown = 1;
	pthread_cond_broadcast(&pool->notify);
	pthread_mutex_unlock(&pool->lock);

	for (size_t i = 0; i < pool->threads_num; ++i) {
		pthread_join(pool->threads[i], NULL);
	}

	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->notify);

	mcl_queue_free(pool->queue);
	free(pool->threads);
	free(pool);
}
