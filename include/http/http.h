#ifndef CWS_HTTP_H
#define CWS_HTTP_H

#define WWW "../www" /**< Directory used to get html files */
/** In the future I'll move conf stuff under a server struct, I can skip just because I want something that works */
#define LOCATION_LEN 1024
#define HTTP_VERSION_LEN 8
#define USER_AGENT_LEN 1024
#define HOST_LEN 1024

enum http_method {
	GET,  /**< GET method */
	POST, /**< POST method */
};
/* In the future I'll add HEAD, PUT, DELETE */

/**
 * @brief HTTP request struct
 *
 */
typedef struct http {
	enum http_method method;			 /**< HTTP request method */
	char location[LOCATION_LEN];		 /**< Resource requested */
	char location_path[LOCATION_LEN];	 /**< Resource path */
	char http_version[HTTP_VERSION_LEN]; /**< HTTP version */
	char user_agent[USER_AGENT_LEN];	 /**< User-Agent */
	char host[HOST_LEN];				 /**< Host */
} http_t;
/* Connection */
/* Accept-Encoding */
/* Accept-Language */

/**
 * @brief Parses a HTTP request
 *
 * @param request[in] The http request sent to the server
 * @return Returns a http_t pointer to the request
 */
http_t *http_parse(char *request_str);

void http_parse_method(http_t *request, const char *method);
void http_send_response(http_t *request, int sockfd);
void http_get_content_type(http_t *request, char *content_type);
void http_free(http_t *request);

#endif