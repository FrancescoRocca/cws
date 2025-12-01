#ifndef CWS_RESPONSE_H
#define CWS_RESPONSE_H

#include "http/request.h"

void cws_http_send_response(cws_request_s *request, cws_http_status_e status);

#endif
