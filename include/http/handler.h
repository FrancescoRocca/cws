#ifndef CWS_HANDLER_H
#define CWS_HANDLER_H

#include "config/config.h"
#include "http/request.h"
#include "http/response.h"

typedef struct cws_handler_config {
	char *root;
	char *domain;
	const char *index;			   /* index file served for "/", NULL = "index.html" */
	const cws_page_s *error_pages; /* custom error pages, or NULL */
	unsigned error_pages_count;
} cws_handler_config_s;

cws_response_s *cws_handler_static_file(cws_request_s *request, cws_handler_config_s *config);

#endif
