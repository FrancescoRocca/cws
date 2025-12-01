#ifndef CWS_NET_H
#define CWS_NET_H

#include <stdbool.h>
#include <sys/socket.h>

#include "utils/error.h"

cws_return cws_fd_set_nonblocking(int sockfd);

void cws_utils_get_client_ip(struct sockaddr_storage *sa, char *ip);

#endif
