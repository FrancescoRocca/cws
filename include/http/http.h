#ifndef CWS_HTTP_H
#define CWS_HTTP_H

#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stddef.h>

#include "utils/config.h"

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

/**
 * @brief HTTP request struct
 *
 */
typedef struct cws_http {
	int sockfd;				  /**< Socket file descriptor */
	cws_http_method_e method; /**< HTTP request method */
	string_s *location;		  /**< Resource requested */
	string_s *location_path;  /**< Full resource path */
	string_s *http_version;	  /**< HTTP version */
	hashmap_s *headers;		  /**< Headers hash map */
} cws_http_s;

/**
 * @brief Parses a HTTP request
 *
 * @param[in] request_str The http request sent to the server
 * @return Returns a http_t pointer to the request
 */
cws_http_s *cws_http_parse(string_s *request_str, int sockfd, cws_config_s *config);

int cws_http_get_content_type(cws_http_s *request, char *content_type);

/**
 * @brief Build the http response
 *
 * @return Returns the size of the response
 */
size_t cws_http_response_builder(char **response, char *http_version, cws_http_status_e status, char *content_type, char *connection, char *body,
								 size_t body_len_bytes);

void cws_http_send_response(cws_http_s *request, cws_http_status_e status);
int cws_http_send_resource(cws_http_s *request);
void cws_http_send_simple_html(cws_http_s *request, cws_http_status_e status, char *title, char *description);

void cws_http_free(cws_http_s *request);

#endif
