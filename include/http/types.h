#ifndef CWS_HTTP_TYPES_H
#define CWS_HTTP_TYPES_H

#include <stddef.h>

/* HTTP Methods */
typedef enum cws_http_method {
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_HEAD,
	HTTP_UNKNOWN,
} cws_http_method_e;

/* HTTP Status Codes */
typedef enum cws_http_status {
	HTTP_OK = 200,
	HTTP_BAD_REQUEST = 400,
	HTTP_NOT_FOUND = 404,
	HTTP_INTERNAL_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
} cws_http_status_e;

const char *cws_http_status_string(cws_http_status_e status);

#endif
