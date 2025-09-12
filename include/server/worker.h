#ifndef CWS_WORKER_H
#define CWS_WORKER_H

#include <myclib/myhashmap.h>
#include <pthread.h>

#include "server/server.h"

typedef struct cws_worker {
	int epfd;
	int pipefd[2];
	size_t clients_num;
	pthread_t thread;
	hashmap_s *clients;
	cws_config_s *config;
} cws_worker_s;

cws_worker_s **cws_worker_init(size_t workers_num, hashmap_s *clients, cws_config_s *config);
void cws_worker_free(cws_worker_s **workers, size_t workers_num);
void *cws_worker_loop(void *arg);

void cws_server_close_client(int epfd, int client_fd, hashmap_s *clients);
cws_server_ret cws_epoll_add(int epfd, int sockfd, uint32_t events);
cws_server_ret cws_epoll_del(int epfd, int sockfd);
cws_server_ret cws_server_handle_client_data(int epfd, int client_fd, hashmap_s *clients, cws_config_s *config);

#endif
