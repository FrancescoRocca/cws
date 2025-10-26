#include "utils/error.h"

#include <stddef.h>

static const cws_error_s errors[] = {
	{CWS_SERVER_OK, "No error"},
	{CWS_SERVER_FD_NONBLOCKING_ERROR, "Failed to set socket as non-blocking"},
	{CWS_SERVER_EPOLL_CREATE_ERROR, "Failed to create epoll instance"},
	{CWS_SERVER_CLIENT_DISCONNECTED_ERROR, "Client disconnected"},
	{CWS_SERVER_HTTP_PARSE_ERROR, "Failed to parse HTTP request"},
	{CWS_SERVER_CONFIG_ERROR, "Invalid server configuration"},
	{CWS_SERVER_GETADDRINFO_ERROR, "getaddrinfo() failed"},
	{CWS_SERVER_SOCKET_ERROR, "Failed to create socket"},
	{CWS_SERVER_SETSOCKOPT_ERROR, "setsockopt() failed"},
	{CWS_SERVER_BIND_ERROR, "bind() failed"},
	{CWS_SERVER_LISTEN_ERROR, "listen() failed"},
	{CWS_SERVER_WORKER_ERROR, "Worker thread initialization failed"},
	{CWS_SERVER_UNKNOWN_ERROR, "Unknown error"}};

const char *cws_error_str(cws_server_ret code) {
	for (size_t i = 0; i < ARR_SIZE(errors); ++i) {
		if (errors[i].code == code)
			return errors[i].msg;
	}
	return "Unrecognized error code";
}
