#include "core/epoll.h"

#include <stdio.h>
#include <sys/epoll.h>

int cws_epoll_add(int epfd, int sockfd) {
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	return status;
}

int cws_epoll_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	return status;
}
