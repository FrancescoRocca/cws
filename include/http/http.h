#ifndef CWS_HTTP_H
#define CWS_HTTP_H

#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stddef.h>

#define CWS_HTTP_HEADER_MAX 512
#define CWS_HTTP_HEADER_CONTENT_MAX 1024

typedef enum cws_http_method {
	CWS_HTTP_GET,
	CWS_HTTP_POST,
	CWS_HTTP_PUT,
	CWS_HTTP_DELETE,
	CWS_HTTP_HEAD,
} cws_http_method_e;

typedef enum cws_http_status {
	CWS_HTTP_OK,
	CWS_HTTP_NOT_FOUND,
	CWS_HTTP_NOT_IMPLEMENTED,
} cws_http_status_e;

typedef struct cws_http {
	int sockfd;
	cws_http_method_e method;
	string_s *location;
	string_s *location_path;
	string_s *http_version;
	hashmap_s *headers;
} cws_http_s;

cws_http_s *cws_http_parse(string_s *request_str, int sockfd);

int cws_http_get_content_type(cws_http_s *request, char *content_type);

size_t cws_http_response_builder(char **response, char *http_version, cws_http_status_e status, char *content_type, char *connection, char *body,
								 size_t body_len_bytes);

void cws_http_send_response(cws_http_s *request, cws_http_status_e status);

int cws_http_send_resource(cws_http_s *request);

void cws_http_send_simple_html(cws_http_s *request, cws_http_status_e status, char *title, char *description);

void cws_http_free(cws_http_s *request);

#endif
