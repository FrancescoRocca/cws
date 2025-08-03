#ifndef CWS_THREADPOOL_H
#define CWS_THREADPOOL_H

#include "myclib/hashmap/myhashmap.h"
#include "server.h"
#include "utils/config.h"

#define CWS_MAX_QUEUE_TASKS 16
#define CWS_MAX_THREADS 16

typedef struct cws_task_t {
	int client_fd;
	int epfd;
	mcl_hashmap *clients;
	cws_config *config;
} cws_task;

typedef struct cws_threadpool_t {
	cws_task queue[CWS_MAX_QUEUE_TASKS];
	int front, rear;

	pthread_mutex_t lock;
	pthread_cond_t cond;
	pthread_t threads[CWS_MAX_THREADS];

	int shutdown;
} cws_threadpool;

cws_threadpool *cws_threadpool_init();
cws_server_ret cws_threadpool_add_task(cws_threadpool *pool, cws_task *task);
cws_server_ret cws_threadpool_worker(cws_threadpool *pool);
cws_server_ret cws_threadpool_destroy(cws_threadpool *pool);

#endif
