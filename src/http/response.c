#include "http/response.h"
#include "http/mime.h"
#include "utils/debug.h"
#include "utils/hash.h"
#include <core/socket.h>
#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "internal/common.h"

static hashmap_s *response_headers_new(void) {
	return hm_new(my_str_hash_fn, my_str_equal_fn, my_str_free_fn, my_str_free_fn, sizeof(char) * HEADER_KEY_MAX,
				  sizeof(char) * HEADER_VALUE_MAX);
}

static int response_get_headers(hashmap_s *headers, char *out_headers, size_t len) {
	size_t keys_len = 0;
	char **keys = (char **)hm_get_keys(headers, &keys_len);
	if (!keys) {
		cws_log_debug("%s", "no headers??");
		return -1;
	}

	size_t offset = 0;
	for (size_t i = 0; i < keys_len; ++i) {
		bucket_s *bucket = hm_get(headers, keys[i]);
		offset += snprintf(out_headers, *out_headers + offset, "%s: %s", keys[i], (char *)bucket->value);
		if ((size_t)(headers + offset) >= len) {
			return -1;
		}
	}

	return 0;
}

cws_response_s *cws_response_new(cws_http_status_e status) {
	cws_response_s *resp = malloc(sizeof(*resp));
	if (!resp) {
		return NULL;
	}

	resp->status = status;
	resp->headers = response_headers_new();
	resp->body_type = RESPONSE_BODY_NONE;
	resp->body_string = NULL;
	resp->body_file = NULL;
	resp->content_length = 0;

	return resp;
}

void cws_response_free(cws_response_s *response) {
	if (!response) {
		return;
	}

	if (response->headers) {
		hm_free(response->headers);
	}

	if (response->body_string) {
		string_free(response->body_string);
	}

	if (response->body_file) {
		fclose(response->body_file);
	}

	free(response);
}

void cws_response_set_header(cws_response_s *response, const char *key, const char *value) {
	if (!response || !key || !value) {
		return;
	}

	char k[HEADER_KEY_MAX], v[HEADER_VALUE_MAX];
	strncpy(k, key, sizeof(k) - 1);
	k[sizeof(k) - 1] = '\0';
	strncpy(v, value, sizeof(v) - 1);
	v[sizeof(v) - 1] = '\0';

	hm_set(response->headers, k, v);
}

void cws_response_set_body_string(cws_response_s *response, const char *body) {
	if (!response || !body) {
		return;
	}

	if (response->body_string) {
		string_free(response->body_string);
	}

	response->body_type = RESPONSE_BODY_STRING;
	response->body_string = string_new(body, strlen(body) + 1);
	response->content_length = strlen(body);
}

void cws_response_set_body_file(cws_response_s *response, const char *filepath) {
	if (!response || !filepath) {
		return;
	}

	FILE *fp = fopen(filepath, "rb");
	if (!fp) {
		cws_log_error("Cannot open file: %s", filepath);
		return;
	}

	struct stat st;
	if (stat(filepath, &st) != 0) {
		fclose(fp);
		return;
	}

	char content_type[CONTENT_TYPE_MAX];
	cws_mime_get_ct(filepath, content_type);
	cws_response_set_header(response, "Content-Type", content_type);

	response->body_type = RESPONSE_BODY_FILE;
	response->body_file = fp;
	response->content_length = st.st_size;
}

cws_response_s *cws_response_html(cws_http_status_e status, const char *title, const char *body) {
	cws_response_s *resp = cws_response_new(status);
	if (!resp) {
		return NULL;
	}

	char html[4096];
	snprintf(html, sizeof(html),
			 "<html>\n"
			 "<head><title>%s</title></head>\n"
			 "<body><h1>%s</h1><p>%s</p></body>\n"
			 "</html>",
			 title, title, body);

	cws_response_set_header(resp, "Content-Type", "text/html");
	cws_response_set_body_string(resp, html);

	return resp;
}

cws_response_s *cws_response_error(cws_http_status_e status, const char *message) {
	const char *status_str = cws_http_status_string(status);
	return cws_response_html(status, status_str, message);
}

int cws_response_send(int sockfd, cws_response_s *response) {
	if (!response) {
		return -1;
	}

	char headers[HEADERS_BUFFER_SIZE];

	response_get_headers(response->headers, headers, sizeof headers);
	int offset = snprintf(headers, sizeof(headers), "HTTP/1.1 %s\r\n", cws_http_status_string(response->status));

	char content_length_str[32];
	snprintf(content_length_str, sizeof(content_length_str), "%zu", response->content_length);

	/* @TODO: I can do this in the response_get_header() but I need to check
	 * if I have space left for \r\n
	 */
	cws_response_set_header(response, "Content-Length", content_length_str);
	offset += snprintf(headers + offset, sizeof(headers) - offset, "Content-Length: %zu\r\n", response->content_length);

	offset += snprintf(headers + offset, sizeof(headers) - offset, "\r\n");

	if (cws_socket_send(sockfd, headers, offset, 0) < 0) {
		return -1;
	}

	if (response->body_type == RESPONSE_BODY_STRING && response->body_string) {
		const char *body = string_cstr(response->body_string);
		cws_socket_send(sockfd, body, response->content_length, 0);
	} else if (response->body_type == RESPONSE_BODY_FILE && response->body_file) {
		char buffer[CHUNK_SIZE];
		size_t bytes_read;
		while ((bytes_read = fread(buffer, 1, sizeof(buffer), response->body_file)) > 0) {
			if (cws_socket_send(sockfd, buffer, bytes_read, 0) < 0) {
				return -1;
			}
		}
	}

	return 0;
}
