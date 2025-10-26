#include "http/request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/debug.h"
#include "utils/hash.h"

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

cws_http_s *cws_http_parse(string_s *request_str) {
	if (!request_str || !request_str->data) {
		return NULL;
	}

	cws_http_s *request = http_new();
	if (request == NULL) {
		return NULL;
	}

	char *str = strdup(request_str->data);
	if (!str) {
		cws_http_free(request);
		return NULL;
	}
	char *str_free = str;

	/* Parse HTTP method */
	str += strspn(str, " ");
	if (*str == '\0') {
		free(str_free);
		cws_http_free(request);
		return NULL;
	}
	size_t len = strcspn(str, " ");
	if (str[len] == '\0') {
		free(str_free);
		cws_http_free(request);
		return NULL;
	}
	str[len] = '\0';
	CWS_LOG_DEBUG("method: %s", str);
	request->method = http_parse_method(str);
	str += len + 1;

	/* Parse location */
	str += strspn(str, " ");
	if (*str == '\0') {
		free(str_free);
		cws_http_free(request);
		return NULL;
	}
	len = strcspn(str, " ");
	if (str[len] == '\0') {
		free(str_free);
		cws_http_free(request);
		return NULL;
	}
	str[len] = '\0';
	string_append(request->location, str);
	str += len + 1;

	/* Adjust location path */
	/* @TODO: fix path traversal */
	string_append(request->location_path, "www");
	if (strcmp(request->location->data, "/") == 0) {
		string_append(request->location_path, "/index.html");
	} else {
		string_append(request->location_path, request->location->data);
	}
	CWS_LOG_DEBUG("location path: %s", request->location_path->data);

	/* Parse HTTP version */
	str += strspn(str, " \t");
	if (*str == '\0') {
		free(str_free);
		cws_http_free(request);
		return NULL;
	}
	len = strcspn(str, "\r\n");
	if (len == 0) {
		free(str_free);
		cws_http_free(request);
		return NULL;
	}
	str[len] = '\0';
	CWS_LOG_DEBUG("version: %s", str);
	string_append(request->http_version, str);
	str += len;

	/* Parse headers until a blank line (\r\n\r\n) */
	request->headers =
		hm_new(my_str_hash_fn, my_str_equal_fn, my_str_free_fn, my_str_free_fn,
			   sizeof(char) * CWS_HTTP_HEADER_MAX, sizeof(char) * CWS_HTTP_HEADER_CONTENT_MAX);

	str += strspn(str, "\r\n");

	while (*str != '\0' && *str != '\r') {
		char *line_end = strstr(str, "\r\n");
		if (!line_end) {
			break;
		}
		*line_end = '\0';

		char *colon = strchr(str, ':');
		if (!colon) {
			str = line_end + 2;
			continue;
		}

		*colon = '\0';

		char *header_key = str;
		char *header_value = colon + 1;
		header_value += strspn(header_value, " \t");

		char hk[CWS_HTTP_HEADER_MAX];
		char hv[CWS_HTTP_HEADER_CONTENT_MAX];

		strncpy(hk, header_key, sizeof(hk) - 1);
		hk[sizeof(hk) - 1] = '\0';

		strncpy(hv, header_value, sizeof(hv) - 1);
		hv[sizeof(hv) - 1] = '\0';

		hm_set(request->headers, hk, hv);

		/* Move to the next line */
		str = line_end + 2;
	}

	free(str_free);

	/* TODO: Parse body */
	/* str is at the beginning of the body */

	return request;
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
