#ifndef CWS_REQUEST_H
#define CWS_REQUEST_H

#include "http/types.h"
#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stddef.h>

typedef struct cws_request {
	cws_http_method_e method;
	string_s *host;
	string_s *path;
	string_s *query_string;
	string_s *http_version;
	hashmap_s *headers;
	string_s *body;
} cws_request_s;

cws_request_s *cws_request_parse(string_s *request_str);

char *cws_request_get_header(cws_request_s *request, const char *header);

void cws_request_free(cws_request_s *request);

#endif
