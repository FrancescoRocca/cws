#ifndef CWS_WORKER_H
#define CWS_WORKER_H

#include <pthread.h>

#include "myclib/hashmap/myhashmap.h"
#include "server/server.h"

typedef struct cws_worker_t {
	int epfd;
	int pipefd[2];
	size_t clients_num;
	pthread_t thread;
	mcl_hashmap *clients;
	cws_config *config;
} cws_worker;

cws_worker **cws_worker_init(size_t workers_num, mcl_hashmap *clients, cws_config *config);
void cws_worker_free(cws_worker **workers, size_t workers_num);
void *cws_worker_loop(void *arg);

void cws_server_close_client(int epfd, int client_fd, mcl_hashmap *clients);
cws_server_ret cws_epoll_add(int epfd, int sockfd, uint32_t events);
cws_server_ret cws_epoll_del(int epfd, int sockfd);
cws_server_ret cws_server_handle_client_data(int epfd, int client_fd, mcl_hashmap *clients, cws_config *config);

#endif
