#ifndef CWS_HANDLER_H
#define CWS_HANDLER_H

#include "http/request.h"
#include "http/response.h"

typedef struct cws_handler_config {
	char *root;
	char *domain;
} cws_handler_config_s;

cws_response_s *cws_handler_static_file(cws_request_s *request, cws_handler_config_s *config);

#endif
