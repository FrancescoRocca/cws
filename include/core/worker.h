#ifndef CWS_WORKER_H
#define CWS_WORKER_H

#include <myclib/myhashmap.h>
#include <pthread.h>
#include <signal.h>

#include "config/config.h"

extern volatile sig_atomic_t cws_server_run;

typedef struct cws_worker {
	int epfd;
	size_t clients_num;
	pthread_t thread;
	cws_config_s *config;
} cws_worker_s;

cws_worker_s **cws_worker_new(size_t workers_num, cws_config_s *config);

void cws_worker_free(cws_worker_s **workers, size_t workers_num);

void *cws_worker_loop(void *arg);

#endif
