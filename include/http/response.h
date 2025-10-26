#ifndef CWS_RESPONSE_H
#define CWS_RESPONSE_H

#include "http/request.h"
#include <sys/types.h>

size_t http_simple_html(char **response, cws_http_status_e status, char *title, char *description);

size_t http_response_builder(char **response, cws_http_status_e status, char *content_type,
							 char *body, size_t body_len_bytes);

void cws_http_send_response(cws_http_s *request, cws_http_status_e status);

#endif
