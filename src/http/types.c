#include "http/types.h"

const char *cws_http_status_string(cws_http_status_e status) {
	switch (status) {
		case HTTP_OK:
			return "200 OK";
		case HTTP_BAD_REQUEST:
			return "400 Bad Request";
		case HTTP_NOT_FOUND:
			return "404 Not Found";
		case HTTP_INTERNAL_ERROR:
			return "500 Internal Server Error";
		case HTTP_NOT_IMPLEMENTED:
			return "501 Not Implemented";
		default:
			return "500 Internal Server Error";
	}
}
