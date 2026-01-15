#include "http/request.h"

#include <myclib/myhashmap.h>
#include <myclib/mystring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/debug.h"
#include "utils/hash.h"

static cws_request_s *http_new(void) {
	cws_request_s *request = malloc(sizeof(*request));
	if (!request) {
		return NULL;
	}
	memset(request, 0, sizeof(*request));

	request->http_version = string_new("", 16);
	request->path = string_new("", 256);
	request->query_string = NULL;
	request->body = NULL;

	return request;
}

static cws_http_method_e http_parse_method(const char *method) {
	if (strcmp(method, "GET") == 0) {
		return HTTP_GET;
	}
	if (strcmp(method, "POST") == 0) {
		return HTTP_POST;
	}
	if (strcmp(method, "PUT") == 0) {
		return HTTP_PUT;
	}
	if (strcmp(method, "DELETE") == 0) {
		return HTTP_DELETE;
	}
	if (strcmp(method, "HEAD") == 0) {
		return HTTP_HEAD;
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
	cws_log_debug("Method: %s", s);
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
	cws_log_debug("Location: %s", s);
	string_append(req->path, s);
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
	cws_log_debug("Version: %s", s);
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

		hm_set(req->headers, hk, hv);

		/* Move to the next line */
		s = line_end + 2;
	}

	*cursor = s;

	return true;
}

cws_request_s *cws_http_parse(string_s *request_str) {
	if (!request_str || !string_cstr(request_str)) {
		return NULL;
	}

	cws_request_s *request = http_new();
	if (!request) {
		return NULL;
	}

	char *str = string_copy(request_str);
	if (!str) {
		cws_http_free(request);
		return NULL;
	}
	char *orig = str;

	/* Parse HTTP method */
	if (!parse_method(request, &str)) {
		free(orig);
		cws_http_free(request);
		return NULL;
	}

	/* Parse location (URL path) */
	if (!parse_location(request, &str)) {
		free(orig);
		cws_http_free(request);
		return NULL;
	}

	/* Parse HTTP version */
	if (!parse_version(request, &str)) {
		free(orig);
		cws_http_free(request);
		return NULL;
	}

	/* Parse headers */
	if (!parse_headers(request, &str)) {
		free(orig);
		cws_http_free(request);
		return NULL;
	}

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

	if (request->path) {
		string_free(request->path);
	}

	if (request->query_string) {
		string_free(request->query_string);
	}

	if (request->body) {
		string_free(request->body);
	}

	free(request);
}
