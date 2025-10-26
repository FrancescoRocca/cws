#ifndef CWS_REQUEST_H
#define CWS_REQUEST_H

#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stddef.h>

#define CWS_HTTP_CONTENT_TYPE 64
#define CWS_HTTP_HEADER_MAX 512
#define CWS_HTTP_HEADER_CONTENT_MAX 1024

typedef enum cws_http_method {
	HTTP_GET,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_HEAD,
	HTTP_UNKNOWN,
} cws_http_method_e;

typedef enum cws_http_status {
	HTTP_OK,
	HTTP_NOT_FOUND,
	HTTP_NOT_IMPLEMENTED,
} cws_http_status_e;

typedef struct cws_request {
	int sockfd;
	cws_http_method_e method;
	string_s *location;
	string_s *location_path;
	string_s *http_version;
	hashmap_s *headers;
} cws_request_s;

cws_request_s *cws_http_parse(string_s *request_str);

void cws_http_free(cws_request_s *request);

#endif
