#ifndef CWS_THREADPOOL_H
#define CWS_THREADPOOL_H

#include "myclib/queue/myqueue.h"
#include "server.h"
#include "utils/config.h"

#define CWS_MAX_QUEUE_TASKS 16
#define CWS_MAX_THREADS 16

typedef struct cws_task_t {
	int client_fd;
	cws_config *config;
} cws_task;

typedef struct cws_thread_task_t {
	void (*function)(void *);
	void *arg;
} cws_thread_task;

typedef struct cws_threadpool_t {
	mcl_queue *queue;
	pthread_mutex_t lock;
	pthread_cond_t notify;
	size_t threads_num;
	pthread_t *threads;
	int shutdown;
} cws_threadpool;

cws_threadpool *cws_threadpool_init(size_t threads_num, size_t queue_size);
cws_server_ret cws_threadpool_submit(cws_threadpool *pool, cws_thread_task *task);
void *cws_threadpool_worker(void *arg);
void cws_threadpool_destroy(cws_threadpool *pool);

#endif
