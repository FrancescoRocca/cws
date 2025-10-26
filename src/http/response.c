#include "http/response.h"

#include "http/mime.h"
#include "http/request.h"
#include "utils/debug.h"
#include <core/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t http_simple_html(char **response, cws_http_status_e status, char *title, char *description) {
	char body[512] = {0};

	snprintf(body, sizeof(body),
			 "<html>\n"
			 "<head>\n"
			 "	<title>%s</title>\n"
			 "</head>\n"
			 "<body>\n"
			 "<p>%s</p>\n"
			 "</body>"
			 "</html>",
			 title, description);
	size_t body_len = strlen(body);

	size_t response_len = http_response_builder(response, status, "text/html", body, body_len);

	return response_len;
}

static size_t http_header_len(char *status_code, char *content_type, size_t body_len) {
	size_t len = snprintf(NULL, 0,
						  "HTTP/1.1 %s\r\n"
						  "Content-Type: %s\r\n"
						  "Content-Length: %zu\r\n"
						  "Connection: close\r\n"
						  "\r\n",
						  status_code, content_type, body_len);

	return len;
}

static char *http_status_string(cws_http_status_e status) {
	switch (status) {
		case HTTP_OK: {
			return "200 OK";
			break;
		}
		case HTTP_NOT_FOUND: {
			return "404 Not Found";
			break;
		}
		case HTTP_NOT_IMPLEMENTED: {
			return "501 Not Implemented";
			break;
		}
	}

	return "?";
}

static size_t file_data(const char *path, char **data) {
	FILE *file = fopen(path, "rb");
	if (!file) {
		return 0;
	}

	/* Retrieve file size */
	fseek(file, 0, SEEK_END);
	const size_t content_length = ftell(file);
	errno = 0;
	rewind(file);
	if (errno != 0) {
		fclose(file);
		CWS_LOG_ERROR("Unable to rewind");

		return 0;
	}

	/* Retrieve file data */
	*data = malloc(content_length);
	if (!*data) {
		fclose(file);
		CWS_LOG_ERROR("Unable to allocate file data");

		return 0;
	}

	/* Read file data */
	size_t read_bytes = fread(*data, 1, content_length, file);
	fclose(file);

	if (read_bytes != content_length) {
		CWS_LOG_ERROR("Partial read from file");

		return 0;
	}

	return content_length;
}

static void http_send_resource(cws_request_s *request) {
	char content_type[CWS_HTTP_CONTENT_TYPE];
	http_get_content_type(request->location_path->data, content_type);

	/* TODO: Check for keep-alive */

	char *data = NULL;
	char *response;
	size_t content_length = file_data(request->location_path->data, &data);

	size_t response_len =
		http_response_builder(&response, HTTP_OK, content_type, data, content_length);

	ssize_t sent = cws_send_data(request->sockfd, response, response_len, 0);
	CWS_LOG_DEBUG("Sent %zd bytes", sent);

	free(response);
	free(data);
}

size_t http_response_builder(char **response, cws_http_status_e status, char *content_type,
							 char *body, size_t body_len_bytes) {
	char *status_code = http_status_string(status);

	size_t header_len = http_header_len(status_code, content_type, body_len_bytes);
	size_t total_len = header_len + body_len_bytes;

	*response = malloc(total_len + 1);
	if (*response == NULL) {
		return 0;
	}

	snprintf(*response, header_len + 1,
			 "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
			 status_code, content_type, body_len_bytes);

	/* Only append body if we have it */
	if (body && body_len_bytes > 0) {
		memcpy(*response + header_len, body, body_len_bytes);
	}

	(*response)[total_len] = '\0';

	return total_len;
}

void cws_http_send_response(cws_request_s *request, cws_http_status_e status) {
	char *response = NULL;

	switch (status) {
		case HTTP_OK:
			http_send_resource(request);
			break;
		case HTTP_NOT_FOUND: {
			size_t len = http_simple_html(&response, HTTP_NOT_FOUND, "404 Not Found",
										  "Resource not found, 404.");
			cws_send_data(request->sockfd, response, len, 0);

			break;
		}
		case HTTP_NOT_IMPLEMENTED: {
			size_t len = http_simple_html(&response, HTTP_NOT_IMPLEMENTED, "501 Not Implemented",
										  "Method not implemented, 501.");
			cws_send_data(request->sockfd, response, len, 0);

			break;
		}
	}

	free(response);
}
