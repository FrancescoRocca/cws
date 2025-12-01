#ifndef CWS_ERROR_H
#define CWS_ERROR_H

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typedef enum cws_ret {
	CWS_OK,
	CWS_FD_NONBLOCKING_ERROR,
	CWS_EPOLL_CREATE_ERROR,
	CWS_CLIENT_DISCONNECTED_ERROR,
	CWS_HTTP_PARSE_ERROR,
	CWS_CONFIG_ERROR,
	CWS_GETADDRINFO_ERROR,
	CWS_SOCKET_ERROR,
	CWS_SETSOCKOPT_ERROR,
	CWS_BIND_ERROR,
	CWS_LISTEN_ERROR,
	CWS_WORKER_ERROR,
	CWS_UNKNOWN_ERROR,
} cws_return;

typedef struct cws_error {
	cws_return code;
	char *msg;
} cws_error_s;

const char *cws_error_str(cws_return code);

#endif
