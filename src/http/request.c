#include "http/request.h"

#include <myclib/mystring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/debug.h"
#include "utils/hash.h"

static cws_request_s *http_new() {
	cws_request_s *request = malloc(sizeof *request);
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

static bool parse_method(cws_request_s *req, char **cursor) {
	char *s = *cursor + strspn(*cursor, " ");
	size_t len = strcspn(s, " ");
	if (len == 0 || s[len] == '\0') {
		return false;
	}

	s[len] = '\0';
	CWS_LOG_DEBUG("method: %s", s);
	req->method = http_parse_method(s);
	*cursor = s + len + 1;

	return true;
}

static bool parse_location(cws_request_s *req, char **cursor) {
	char *s = *cursor + strspn(*cursor, " ");
	size_t len = strcspn(s, " ");
	if (len == 0 || s[len] == '\0') {
		return false;
	}

	s[len] = '\0';
	CWS_LOG_DEBUG("location: %s", s);
	string_append(req->location, s);
	*cursor = s + len + 1;

	return true;
}

static bool parse_version(cws_request_s *req, char **cursor) {
	char *s = *cursor + strspn(*cursor, " \t");
	size_t len = strcspn(s, "\r\n");
	if (len == 0 || s[len] == '\0') {
		return false;
	}

	s[len] = '\0';
	CWS_LOG_DEBUG("version: %s", s);
	string_append(req->http_version, s);
	*cursor = s + len + 1;

	return true;
}

static bool parse_headers(cws_request_s *req, char **cursor) {
	req->headers = hm_new(my_str_hash_fn, my_str_equal_fn, my_str_free_fn, my_str_free_fn,
						  sizeof(char) * CWS_HTTP_HEADER_MAX, sizeof(char) * CWS_HTTP_HEADER_CONTENT_MAX);

	char *s = *cursor + strspn(*cursor, "\r\n");
	while (*s != '\0' && *s != '\r') {
		char *line_end = strstr(s, "\r\n");
		if (!line_end) {
			break;
		}
		*line_end = '\0';

		char *colon = strchr(s, ':');
		if (!colon) {
			s = line_end + 2;
			continue;
		}

		*colon = '\0';

		char *header_key = s;
		char *header_value = colon + 1;
		header_value += strspn(header_value, " \t");

		char hk[CWS_HTTP_HEADER_MAX];
		char hv[CWS_HTTP_HEADER_CONTENT_MAX];

		strncpy(hk, header_key, sizeof(hk) - 1);
		hk[sizeof(hk) - 1] = '\0';

		strncpy(hv, header_value, sizeof(hv) - 1);
		hv[sizeof(hv) - 1] = '\0';

		// CWS_LOG_DEBUG("%s:%s", hk, hv);
		hm_set(req->headers, hk, hv);

		/* Move to the next line */
		s = line_end + 2;
	}

	*cursor = s;

	return true;
}

cws_request_s *cws_http_parse(string_s *request_str) {
	if (!request_str || !request_str->data) {
		return NULL;
	}

	cws_request_s *request = http_new();
	if (!request) {
		return NULL;
	}

	char *str = strdup(request_str->data);
	if (!str) {
		cws_http_free(request);
		return NULL;
	}
	char *orig = str;

	/* Parse HTTP method */
	parse_method(request, &str);

	/* Parse location */
	parse_location(request, &str);

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
	parse_version(request, &str);

	/* Parse headers */
	parse_headers(request, &str);

	/* TODO: Parse body */
	/* orig is at the beginning of the body */

	/* Free the original string */
	free(orig);

	return request;
}

void cws_http_free(cws_request_s *request) {
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
