#ifndef CWS_EPOLL_UTILS_H
#define CWS_EPOLL_UTILS_H

int cws_epoll_add(int epfd, int sockfd);

int cws_epoll_del(int epfd, int sockfd);

int cws_epoll_create_with_fd(int fd);

#endif
