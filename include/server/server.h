#ifndef CWS_SERVER_H
#define CWS_SERVER_H

#include <myclib/myhashmap.h>
#include <myclib/mysocket.h>
#include <signal.h>

#include "config/config.h"
#include "utils/utils.h"
#include "worker.h"

/* Clients max queue */
#define CWS_SERVER_BACKLOG 128

/* Size of the epoll_event array */
#define CWS_SERVER_EPOLL_MAXEVENTS 64

#define CWS_SERVER_EPOLL_TIMEOUT 3000

#define CWS_SERVER_MAX_REQUEST_SIZE (16 * 1024) /* 16KB */

#define CWS_WORKERS_NUM 6

/* Main server loop */
extern volatile sig_atomic_t cws_server_run;

typedef struct cws_server {
	int epfd;
	int sockfd;
	cws_worker_s **workers;
	cws_config_s *config;
} cws_server_s;

cws_server_ret cws_server_setup(cws_server_s *server, cws_config_s *config);

cws_server_ret cws_server_start(cws_server_s *server);

void cws_server_shutdown(cws_server_s *server);

int cws_server_handle_new_client(int server_fd);

int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa);

#endif
