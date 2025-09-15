#define _XOPEN_SOURCE 1

#include "http/http.h"

#include <fcntl.h>
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

	request->http_version = string_new("", 16);
	request->location = string_new("", 128);
	request->location_path = string_new("", 512);

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
		return NULL;
	}

	/* Retrieve file size */
	fseek(file, 0, SEEK_END);
	const size_t content_length = ftell(file);
	rewind(file);

	/* Retrieve file data */
	*data = malloc(content_length);
	if (!data) {
		fclose(file);
		CWS_LOG_ERROR("Unable to allocate file data");

		return NULL;
	}

	/* Read file data */
	size_t read_bytes = fread(data, 1, content_length, file);
	fclose(file);

	if (read_bytes != content_length) {
		free(data);
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

	char *data;
	char *response;
	size_t content_length = file_data(request->location_path->data, &data);

	size_t response_len = http_response_builder(&response, "HTTP/1.1", HTTP_OK, content_type, data, content_length);

	ssize_t sent = send(request->sockfd, response, response_len, MSG_NOSIGNAL);
	CWS_LOG_DEBUG("Sent %zu bytes", sent);

	free(response);
	free(data);

	return CWS_SERVER_OK;
}

static void http_send_simple_html(cws_http_s *request, cws_http_status_e status, char *title, char *description) {
	char body[512];
	memset(body, 0, sizeof(body));

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

	char conn[32] = "keep-alive";
	bucket_s *connection = hm_get(request->headers, "Connection");
	if (connection) {
		strncpy(conn, (char *)connection->value, sizeof(conn) - 1);
		conn[sizeof(conn) - 1] = '\0';
	}
	hm_free_bucket(connection);

	char *response = NULL;
	size_t response_len = http_response_builder(&response, "HTTP/1.1", status, "text/html", body, body_len);

	ssize_t sent = send(request->sockfd, response, response_len, MSG_NOSIGNAL);
	if (sent < 0) {
		CWS_LOG_ERROR("Failed to send response");
	}

	free(response);
}

cws_http_s *cws_http_parse(string_s *request_str) {
	if (!request_str) {
		return NULL;
	}

	cws_http_s *request = http_new();
	if (request == NULL) {
		return NULL;
	}

	char *request_str_cpy = strdup(string_cstr(request_str));
	if (!request_str_cpy) {
		free(request);

		return NULL;
	}

	char *saveptr = NULL;
	char *pch = NULL;

	/* Parse HTTP method */
	pch = strtok_r(request_str_cpy, " ", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_str_cpy);

		return NULL;
	}
	// CWS_LOG_DEBUG("method: %s", pch);

	request->method = http_parse_method(pch);
	// TODO: fix here
	if (ret < 0) {
		/* Not implemented */
		cws_http_free(request);
		free(request_str_cpy);

		cws_http_send_response(request, HTTP_NOT_IMPLEMENTED);

		return NULL;
	}

	/* Parse location */
	pch = strtok_r(NULL, " ", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_str_cpy);

		return NULL;
	}
	// CWS_LOG_DEBUG("location: %s", pch);
	string_append(request->location, pch);
	// TODO: mcl_string_append(request->location_path, config->www);

	/* Adjust location path */
	if (strcmp(string_cstr(request->location), "/") == 0) {
		string_append(request->location_path, "/index.html");
	} else {
		string_append(request->location_path, string_cstr(request->location));
	}
	// CWS_LOG_DEBUG("location path: %s", mcl_string_cstr(request->location_path));

	/* Parse HTTP version */
	pch = strtok_r(NULL, " \r\n", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_str_cpy);

		return NULL;
	}
	// CWS_LOG_DEBUG("version: %s", pch);
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

	/* TODO: Parse body */

	free(request_str_cpy);

	return request;
}

size_t http_response_builder(char **response, char *http_version, cws_http_status_e status, char *content_type, char *body, size_t body_len_bytes) {
	char *status_code = http_status_string(status);

	size_t header_len = snprintf(NULL, 0,
								 "%s %s\r\n"
								 "Content-Type: %s\r\n"
								 "Content-Length: %ld\r\n"
								 "\r\n",
								 http_version, status_code, content_type, body_len_bytes);

	size_t total_len = header_len + body_len_bytes;

	*response = malloc(total_len);
	if (*response == NULL) {
		return 0;
	}

	snprintf(*response, header_len + 1, "%s %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", http_version, status_code, content_type, body_len_bytes);

	/* Only append body if we have it */
	if (body && body_len_bytes > 0) {
		memcpy(*response + header_len, body, body_len_bytes);
	}

	return total_len;
}

void cws_http_send_response(cws_http_s *request, cws_http_status_e status) {
	switch (status) {
		case HTTP_OK:
			http_send_resource(request);
			break;
		case HTTP_NOT_FOUND: {
			http_send_simple_html(request, HTTP_NOT_FOUND, "404 Not Found", "Resource not found, 404.");
			break;
		}
		case HTTP_NOT_IMPLEMENTED: {
			http_send_simple_html(request, HTTP_NOT_IMPLEMENTED, "501 Not Implemented", "Method not implemented, 501.");
			break;
		}
	}
}

void cws_http_free(cws_http_s *request) {
	hm_free(request->headers);
	string_free(request->http_version);
	string_free(request->location);
	string_free(request->location_path);
	free(request);
}
