#ifndef CWS_HTTP_H
#define CWS_HTTP_H

#include <stddef.h>

#include "utils/config.h"
#include "utils/hashmap.h"

#define CWS_HTTP_LOCATION_LEN 512
#define CWS_HTTP_LOCATION_PATH_LEN 1024
#define CWS_HTTP_VERSION_LEN 8

typedef enum cws_http_method_t {
	CWS_HTTP_GET,  /**< GET method */
	CWS_HTTP_POST, /**< POST method */
	CWS_HTTP_PUT,
	CWS_HTTP_DELETE,
	CWS_HTTP_HEAD,
} cws_http_method;

typedef enum cws_http_status_t {
	CWS_HTTP_OK,
	CWS_HTTP_NOT_FOUND,
	CWS_HTTP_NOT_IMPLEMENTED,
} cws_http_status;

/**
 * @brief HTTP request struct
 *
 */
typedef struct cws_http_t {
	int sockfd;										/**< Socket file descriptor */
	cws_http_method method;							/**< HTTP request method */
	char location[CWS_HTTP_LOCATION_LEN];			/**< Resource requested */
	char location_path[CWS_HTTP_LOCATION_PATH_LEN]; /**< Full resource path */
	char http_version[CWS_HTTP_VERSION_LEN];		/**< HTTP version */
	mcl_hashmap *headers;							/**< Headers hash map */
} cws_http;
/* Connection */
/* Accept-Encoding */
/* Accept-Language */

/**
 * @brief Parses a HTTP request
 *
 * @param[in] request_str The http request sent to the server
 * @return Returns a http_t pointer to the request
 */
cws_http *cws_http_parse(char *request_str, int sockfd, cws_config *config);

int cws_http_parse_method(cws_http *request, const char *method);
int cws_http_get_content_type(cws_http *request, char *content_type);
char *cws_http_status_string(cws_http_status status);

/**
 * @brief Build the http response
 *
 * @return Returns the size of the response
 */
size_t cws_http_response_builder(char **response, char *http_version, cws_http_status status, char *content_type, char *connection, char *body,
								 size_t body_len_bytes);

void cws_http_send_response(cws_http *request, cws_http_status status);
void cws_http_send_resource(cws_http *request);
void cws_http_send_simple_html(cws_http *request, cws_http_status status, char *title, char *description);

void cws_http_free(cws_http *request);

#endif