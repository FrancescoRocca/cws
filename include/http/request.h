#ifndef CWS_REQUEST_H
#define CWS_REQUEST_H

#include "http/types.h"
#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stddef.h>

#define CWS_HTTP_CONTENT_TYPE 64
#define CWS_HTTP_HEADER_MAX 512
#define CWS_HTTP_HEADER_CONTENT_MAX 1024

typedef struct cws_request {
	cws_http_method_e method;
	string_s *path;
	string_s *query_string;
	string_s *http_version;
	hashmap_s *headers;
	string_s *body;
} cws_request_s;

cws_request_s *cws_http_parse(string_s *request_str);

void cws_http_free(cws_request_s *request);

#endif
