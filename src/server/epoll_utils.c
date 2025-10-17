#include "server/epoll_utils.h"

#include <sys/epoll.h>

#include "utils/debug.h"
#include "utils/utils.h"

cws_server_ret cws_epoll_add(int epfd, int sockfd) {
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = sockfd;
	const int status = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_add()");
		return CWS_SERVER_EPOLL_ADD_ERROR;
	}

	return CWS_SERVER_OK;
}

cws_server_ret cws_epoll_del(int epfd, int sockfd) {
	const int status = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);

	if (status != 0) {
		CWS_LOG_ERROR("epoll_ctl_del()");
		return CWS_SERVER_EPOLL_DEL_ERROR;
	}

	return CWS_SERVER_OK;
}

int cws_epoll_create_with_fd(int fd) {
	int epfd = epoll_create1(0);
	if (epfd == -1) {
		return -1;
	}
	if (cws_epoll_add(epfd, fd) != CWS_SERVER_OK) {
		return -1;
	}

	return epfd;
}
