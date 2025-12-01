#include "http/response.h"

#include "http/mime.h"
#include "http/request.h"
#include "utils/debug.h"
#include <core/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* 8 KB reading buffer length */
#define CHUNK_SIZE 8192
#define HEADERS_LEN 512

static size_t http_header_len(const char *status_code, char *content_type, size_t body_len) {
	size_t len = snprintf(NULL, 0,
						  "HTTP/1.1 %s\r\n"
						  "Content-Type: %s\r\n"
						  "Content-Length: %zu\r\n"
						  "Connection: close\r\n"
						  "\r\n",
						  status_code, content_type, body_len);

	return len;
}

static const char *http_status_string(cws_http_status_e status) {
	switch (status) {
		case HTTP_OK: {
			return "200 OK";
		}
		case HTTP_NOT_FOUND: {
			return "404 Not Found";
		}
		case HTTP_NOT_IMPLEMENTED: {
			return "501 Not Implemented";
		}
	}

	return "?";
}

static void http_send_headers(int sockfd, const char *status_str, const char *content_type, size_t content_length) {
	char headers[HEADERS_LEN];
	int len = snprintf(headers, sizeof headers,
					   "HTTP/1.1 %s\r\n"
					   "Content-Type: %s\r\n"
					   "Content-Length: %zu\r\n"
					   "Connection: close\r\n"
					   "\r\n",
					   status_str, content_type, content_length);

	if (len > 0) {
		cws_send_data(sockfd, headers, len, 0);
	}
}

static void http_send_file(cws_request_s *request) {
	const char *path = request->location_path->data;
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		CWS_LOG_ERROR("Cannot open file: %s", path);
		cws_http_send_response(request, HTTP_NOT_FOUND);
		return;
	}

	/* Get file size */
	struct stat st;
	if (stat(path, &st) != 0) {
		fclose(fp);
		cws_http_send_response(request, HTTP_NOT_FOUND);
		return;
	}
	size_t file_size = st.st_size;

	char content_tye[CWS_HTTP_CONTENT_TYPE];
	http_get_content_type(path, content_tye);

	/* Send headers */
	http_send_headers(request->sockfd, http_status_string(HTTP_OK), content_tye, file_size);

	/* Send the file in chunks */
	char buffer[CHUNK_SIZE];
	size_t bytes_read = 0;

	while ((bytes_read = fread(buffer, 1, sizeof buffer, fp)) > 0) {
		/* TODO: check return */
		cws_send_data(request->sockfd, buffer, bytes_read, 0);
	}

	fclose(fp);
	CWS_LOG_DEBUG("Served file: %s (%zu bytes)", path, file_size);
}

void http_send_simple_html(cws_request_s *request, cws_http_status_e status, const char *title, const char *desc) {
	const char *fmt = "<html>\n"
					  "<head><title>%s</title></head>\n"
					  "<body><p>%s</p></body>\n"
					  "</html>";

	int body_len = snprintf(NULL, 0, fmt, title, desc);
	if (body_len < 0)
		body_len = 0;

	http_send_headers(request->sockfd, http_status_string(status), "text/html", body_len);

	char *body = malloc(body_len + 1);
	if (body) {
		snprintf(body, body_len + 1, fmt, title, desc);
		cws_send_data(request->sockfd, body, body_len, 0);
		free(body);
	}
}

size_t http_response_builder(char **response, cws_http_status_e status, char *content_type, char *body,
							 size_t body_len_bytes) {
	const char *status_code = http_status_string(status);

	size_t header_len = http_header_len(status_code, content_type, body_len_bytes);
	size_t total_len = header_len + body_len_bytes;

	*response = malloc(total_len + 1);
	if (*response == NULL) {
		return 0;
	}

	snprintf(*response, header_len + 1,
			 "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", status_code,
			 content_type, body_len_bytes);

	/* Only append body if we have it */
	if (body && body_len_bytes > 0) {
		memcpy(*response + header_len, body, body_len_bytes);
	}

	(*response)[total_len] = '\0';

	return total_len;
}

void cws_http_send_response(cws_request_s *request, cws_http_status_e status) {
	switch (status) {
		case HTTP_OK:
			http_send_file(request);
			break;

		case HTTP_NOT_FOUND:
			http_send_simple_html(request, HTTP_NOT_FOUND, "404 Not Found", "The requested resource was not found.");
			break;

		case HTTP_NOT_IMPLEMENTED:
			http_send_simple_html(request, HTTP_NOT_IMPLEMENTED, "501 Not Implemented", "Method not implemented.");
			break;

		default:
			http_send_simple_html(request, status, "Error", "An unexpected error occurred.");
			break;
	}
}
