#ifndef CWS_ERROR_H
#define CWS_ERROR_H

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typedef enum cws_server_ret {
	CWS_SERVER_OK,
	CWS_SERVER_FD_NONBLOCKING_ERROR,
	CWS_SERVER_EPOLL_CREATE_ERROR,
	CWS_SERVER_CLIENT_DISCONNECTED_ERROR,
	CWS_SERVER_HTTP_PARSE_ERROR,
	CWS_SERVER_CONFIG,
	CWS_SERVER_GETADDRINFO_ERROR,
	CWS_SERVER_SOCKET_ERROR,
	CWS_SERVER_SETSOCKOPT_ERROR,
	CWS_SERVER_BIND_ERROR,
	CWS_SERVER_LISTEN_ERROR,
	CWS_SERVER_WORKER_ERROR,
} cws_server_ret;

typedef struct cws_error {
	cws_server_ret code;
	char *str;
} cws_error_s;

char *cws_error_str(cws_server_ret code);

#endif
