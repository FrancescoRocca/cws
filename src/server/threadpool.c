#include "server/threadpool.h"

#include <stdlib.h>

cws_threadpool *cws_threadpool_init() {
	cws_threadpool *pool = malloc(sizeof(cws_threadpool));
	if (pool == NULL) {
		return NULL;
	}

	int ret;

	ret = pthread_mutex_init(&pool->lock, NULL);
	if (ret != 0) {
		free(pool);
		return NULL;
	}

	ret = pthread_cond_init(&pool->cond, NULL);
	if (ret != 0) {
		free(pool);
		return NULL;
	}

	pool->front = 0;
	pool->rear = 0;
	pool->shutdown = 0;

	return pool;
}

cws_server_ret cws_threadpool_add_task(cws_threadpool *pool, cws_task *task) {
	pthread_mutex_lock(&pool->lock);

	/* ? */

	pthread_mutex_unlock(&pool->lock);

	return CWS_SERVER_OK;
}

cws_server_ret cws_threadpool_worker(cws_threadpool *pool) { return CWS_SERVER_OK; }

cws_server_ret cws_threadpool_destroy(cws_threadpool *pool) {
	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->cond);

	return CWS_SERVER_OK;
}
