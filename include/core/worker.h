#ifndef CWS_WORKER_H
#define CWS_WORKER_H

#include <myclib/myhashmap.h>
#include <pthread.h>
#include <signal.h>

#include "config/config.h"

/* Blocking timeout for epoll_wait in ms */
#define WORKER_EPOLL_TIMEOUT 250

/* Max number of epoll events processed per iteration */
#define WORKER_EPOLL_MAX_EVENTS 64

extern volatile sig_atomic_t cws_server_run;

typedef struct cws_worker {
	int epfd;
	pthread_t thread;
	cws_config_s *config;
} cws_worker_s;

cws_worker_s **cws_worker_new(size_t workers_num, cws_config_s *config);

void cws_worker_free(cws_worker_s **workers, size_t workers_num);

#endif
