#ifndef CWS_SERVER_H
#define CWS_SERVER_H

#include <myclib/myhashmap.h>
#include <netdb.h>
#include <signal.h>

#include "config/config.h"
#include "core/worker.h"
#include "utils/error.h"

/* Maximum queue of pending client connections */
#define CWS_SERVER_BACKLOG 128

/* Max number of epoll events processed per iteration */
#define CWS_SERVER_EPOLL_MAXEVENTS 64

/* Blocking timeout for epoll_wait in ms */
#define CWS_SERVER_EPOLL_TIMEOUT 3000

/* Maximum allowed HTTP request size */
#define CWS_SERVER_MAX_REQUEST_SIZE (16 * 1024) /* 16KB */

/* Number of worker threads */
#define CWS_WORKERS_NUM 6

/* Global flag used to stop server */
extern volatile sig_atomic_t cws_server_run;

typedef struct cws_server {
	int epfd;				/* epoll instance for incoming connections */
	int sockfd;				/* listening socket */
	cws_worker_s **workers; /* worker thread pool */
	cws_config_s *config;	/* config pointer */
} cws_server_s;

cws_return cws_server_setup(cws_server_s *server, cws_config_s *config);

cws_return cws_server_start(cws_server_s *server);

void cws_server_shutdown(cws_server_s *server);

int cws_server_handle_new_client(int server_fd);

int cws_server_accept_client(int server_fd, struct sockaddr_storage *their_sa);

#endif
