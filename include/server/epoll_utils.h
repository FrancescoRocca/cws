#include "utils/utils.h"

cws_server_ret cws_epoll_add(int epfd, int sockfd);

cws_server_ret cws_epoll_del(int epfd, int sockfd);

int cws_epoll_create_with_fd(int fd);
