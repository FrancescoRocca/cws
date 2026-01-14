#ifndef CWS_RESPONSE_H
#define CWS_RESPONSE_H

#include "http/types.h"
#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stddef.h>
#include <stdio.h>

typedef enum cws_response_body_type {
	RESPONSE_BODY_NONE,
	RESPONSE_BODY_STRING,
	RESPONSE_BODY_FILE,
} cws_response_body_type_e;

typedef struct cws_response {
	cws_http_status_e status;
	hashmap_s *headers;

	/* Body handling */
	cws_response_body_type_e body_type;
	string_s *body_string;
	FILE *body_file;
	size_t content_length;
} cws_response_s;

cws_response_s *cws_response_new(cws_http_status_e status);
void cws_response_free(cws_response_s *response);

void cws_response_set_header(cws_response_s *response, const char *key, const char *value);
void cws_response_set_body_string(cws_response_s *response, const char *body);
void cws_response_set_body_file(cws_response_s *response, const char *filepath);

cws_response_s *cws_response_html(cws_http_status_e status, const char *title, const char *body);
cws_response_s *cws_response_error(cws_http_status_e status, const char *message);

int cws_response_send(int sockfd, cws_response_s *response);

#endif
