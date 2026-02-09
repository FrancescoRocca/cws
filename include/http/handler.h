#ifndef CWS_HANDLER_H
#define CWS_HANDLER_H

#include "http/request.h"
#include "http/response.h"

/* Configuration for static file serving */
typedef struct cws_handler_config {
	const char *root_dir;
	const char *index_file;
} cws_handler_config_s;

/* Static file handler */
cws_response_s *cws_handler_static_file(cws_request_s *request, cws_handler_config_s *config);

/* Error handlers */
cws_response_s *cws_handler_not_found(cws_request_s *request);
cws_response_s *cws_handler_not_implemented(cws_request_s *request);

#endif
