#define _XOPEN_SOURCE 1

#include "http/http.h"

#include <errno.h>
#include <fcntl.h>
#include <myclib/mysocket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/debug.h"
#include "utils/utils.h"

static cws_http_s *http_new() {
	cws_http_s *request = malloc(sizeof(cws_http_s));
	if (!request) {
		return NULL;
	}
	memset(request, 0, sizeof *request);

	request->http_version = string_new("", 16);
	request->location = string_new("", 128);
	request->location_path = string_new("", 128);

	return request;
}

static cws_http_method_e http_parse_method(const char *method) {
	if (strcmp(method, "GET") == 0) {
		return HTTP_GET;
	}

	if (strcmp(method, "POST") == 0) {
		return HTTP_POST;
	}

	return HTTP_UNKNOWN;
}

static int http_get_content_type(char *location_path, char *content_type) {
	/* Find last occurrence of a string */
	char *ptr = strrchr(location_path, '.');
	if (ptr == NULL) {
		return -1;
	}
	ptr += 1;

	char ct[32] = {0};

	if (strcmp(ptr, "html") == 0 || strcmp(ptr, "css") == 0 || strcmp(ptr, "javascript") == 0) {
		strncpy(ct, "text", sizeof ct);
	}

	if (strcmp(ptr, "jpg") == 0 || strcmp(ptr, "png") == 0) {
		strncpy(ct, "image", sizeof ct);
	}

	snprintf(content_type, 1024, "%s/%s", ct, ptr);

	return 0;
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

static cws_server_ret http_send_resource(cws_http_s *request) {
	/* Retrieve correct Content-Type */
	char content_type[1024];
	http_get_content_type(request->location_path->data, content_type);

	/* TODO: Check for keep-alive */

	char *data = NULL;
	char *response;
	size_t content_length = file_data(request->location_path->data, &data);

	size_t response_len = http_response_builder(&response, HTTP_OK, content_type, data, content_length);

	size_t total_sent = 0;
	while (total_sent < response_len) {
		ssize_t sent = send(request->sockfd, response + total_sent, response_len - total_sent, MSG_NOSIGNAL);
		if (sent < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			break;
		}
		total_sent += sent;
	}
	CWS_LOG_DEBUG("Sent %zd bytes", total_sent);

	free(response);
	free(data);

	return CWS_SERVER_OK;
}

static size_t http_simple_html(char **response, cws_http_status_e status, char *title, char *description) {
	char body[512];
	memset(body, 0, sizeof body);

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

cws_http_s *cws_http_parse(string_s *request_str) {
	if (!request_str) {
		return NULL;
	}

	cws_http_s *request = http_new();
	if (request == NULL) {
		return NULL;
	}

	char *request_copy = strdup(request_str->data);
	if (request_copy == NULL) {
		cws_http_free(request);
		return NULL;
	}

	char *saveptr = NULL;
	char *pch = NULL;

	/* Parse HTTP method */
	pch = strtok_r(request_copy, " ", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_copy);
		return NULL;
	}
	CWS_LOG_DEBUG("method: %s", pch);

	request->method = http_parse_method(pch);
	if (request->method == HTTP_UNKNOWN) {
		cws_http_free(request);
		free(request_copy);
		return NULL;
	}

	/* Parse location */
	pch = strtok_r(NULL, " ", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_copy);
		return NULL;
	}
	string_append(request->location, pch);

	/* Adjust location path */
	string_append(request->location_path, "www");

	if (strcmp(request->location->data, "/") == 0) {
		string_append(request->location_path, "/index.html");
	} else {
		string_append(request->location_path, request->location->data);
	}
	CWS_LOG_DEBUG("location path: %s", request->location_path->data);

	/* Parse HTTP version */
	pch = strtok_r(NULL, " \r\n", &saveptr);
	if (pch == NULL) {
		free(request_copy);
		cws_http_free(request);

		return NULL;
	}
	CWS_LOG_DEBUG("version: %s", pch);
	string_append(request->http_version, pch);

	/* Parse headers until a \r\n */
	request->headers =
		hm_new(my_str_hash_fn, my_str_equal_fn, my_str_free_fn, my_str_free_fn, sizeof(char) * CWS_HTTP_HEADER_MAX, sizeof(char) * CWS_HTTP_HEADER_CONTENT_MAX);
	char *header_colon;
	while (pch) {
		/* Get header line */
		pch = strtok_r(NULL, "\r\n", &saveptr);
		if (pch == NULL) {
			break;
		}
		/* Find ":" */
		header_colon = strchr(pch, ':');
		if (header_colon != NULL) {
			*header_colon = '\0';
		} else {
			break;
		}

		char hk[CWS_HTTP_HEADER_MAX];
		char hv[CWS_HTTP_HEADER_CONTENT_MAX];

		/* Header key */
		strncpy(hk, pch, sizeof(hk));
		/* Header value (starting from ": ") */
		char *hvalue = header_colon + 2;
		strncpy(hv, hvalue, sizeof(hv));

		// CWS_LOG_DEBUG("hkey: %s -> %s", hk, hv);
		hm_set(request->headers, hk, hv);
	}

	free(request_copy);

	/* TODO: Parse body */

	return request;
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

size_t http_response_builder(char **response, cws_http_status_e status, char *content_type, char *body, size_t body_len_bytes) {
	char *status_code = http_status_string(status);

	size_t header_len = http_header_len(status_code, content_type, body_len_bytes);
	size_t total_len = header_len + body_len_bytes;

	*response = malloc(total_len + 1);
	if (*response == NULL) {
		return 0;
	}

	snprintf(*response, header_len + 1, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", status_code, content_type,
			 body_len_bytes);
	// CWS_LOG_DEBUG("response: %s", *response);

	/* Only append body if we have it */
	if (body && body_len_bytes > 0) {
		memcpy(*response + header_len, body, body_len_bytes);
	}
	// CWS_LOG_DEBUG("response: %s", *response);

	(*response)[total_len] = '\0';

	return total_len;
}

void cws_http_send_response(cws_http_s *request, cws_http_status_e status) {
	char *response = NULL;

	switch (status) {
		case HTTP_OK:
			http_send_resource(request);
			break;
		case HTTP_NOT_FOUND: {
			size_t len = http_simple_html(&response, HTTP_NOT_FOUND, "404 Not Found", "Resource not found, 404.");
			sock_writeall(request->sockfd, response, len);

			break;
		}
		case HTTP_NOT_IMPLEMENTED: {
			size_t len = http_simple_html(&response, HTTP_NOT_IMPLEMENTED, "501 Not Implemented", "Method not implemented, 501.");
			sock_writeall(request->sockfd, response, len);

			break;
		}
	}

	free(response);
}

void cws_http_free(cws_http_s *request) {
	if (!request) {
		return;
	}

	if (request->headers) {
		hm_free(request->headers);
	}

	if (request->http_version) {
		string_free(request->http_version);
	}

	if (request->location) {
		string_free(request->location);
	}

	if (request->location_path) {
		string_free(request->location_path);
	}

	free(request);
}
