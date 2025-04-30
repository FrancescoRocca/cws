#ifndef CWS_HTTP_H
#define CWS_HTTP_H

#include "utils/hashmap.h"

#define CWS_WWW "../www" /**< Directory used to get html files */
/** In the future I'll move conf stuff under a server struct, I can skip just because I want something that works */
#define CWS_HTTP_LOCATION_LEN 512
#define CWS_HTTP_LOCATION_PATH_LEN 1024
#define CWS_HTTP_VERSION_LEN 8

typedef enum cws_http_method_t {
	CWS_HTTP_GET,  /**< GET method */
	CWS_HTTP_POST, /**< POST method */
	CWS_HTTP_PUT,
	CWS_HTTP_DELETE,
} cws_http_method;
/* In the future I'll add HEAD, PUT, DELETE */

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
	cws_hashmap *headers;							/**< Headers hash map */
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
cws_http *cws_http_parse(char *request_str, int sockfd);

void cws_http_parse_method(cws_http *request, const char *method);
void cws_http_get_content_type(cws_http *request, char *content_type);

void cws_http_send_response(cws_http *request);
void cws_http_send_not_found(cws_http *request);
void cws_http_send_not_implemented(cws_http *request);

void cws_http_free(cws_http *request);

#endif