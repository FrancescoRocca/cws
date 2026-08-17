#include "http/handler.h"
#include "utils/debug.h"
#include <myclib/mystring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Sanitize and resolve file path */
static string_s *resolve_file_path(const char *url_path, cws_handler_config_s *config) {
	string_s *full_path = string_new(config->root, 0);
	if (!full_path) {
		return NULL;
	}

	if (strcmp(url_path, "/") == 0) {
		string_append(full_path, "/");
		string_append(full_path, config->index ? config->index : "index.html");
		return full_path;
	}

	string_s *url_path_string = string_new(url_path, 0);
	if (!url_path_string) {
		string_free(full_path);
		return NULL;
	}

	/* Block directory traversal attempts */
	if (string_find(url_path_string, "..") >= 0) {
		string_free(url_path_string);
		string_free(full_path);
		return NULL;
	}

	string_append(full_path, url_path);
	string_free(url_path_string);

	return full_path;
}

static bool file_exists(const char *filepath) {
	struct stat st;
	return stat(filepath, &st) == 0 && S_ISREG(st.st_mode);
}

static cws_response_s *cws_handler_error_page(cws_handler_config_s *config, cws_http_status_e status) {
	char status_str[16];
	snprintf(status_str, sizeof status_str, "%d", (int)status);

	for (unsigned i = 0; i < config->error_pages_count; ++i) {
		if (strcmp(config->error_pages[i].status, status_str) != 0) {
			continue;
		}

		const char *path = config->error_pages[i].path;
		if (!file_exists(path)) {
			return NULL;
		}

		cws_response_s *resp = cws_response_new(status);
		if (!resp) {
			return NULL;
		}
		cws_response_set_body_file(resp, path);
		return resp;
	}

	return NULL;
}

static cws_response_s *cws_handler_error(cws_handler_config_s *config, cws_http_status_e status, const char *message) {
	if (config) {
		cws_response_s *custom = cws_handler_error_page(config, status);
		if (custom) {
			return custom;
		}
	}
	return cws_response_error(status, message);
}

static cws_response_s *cws_handler_not_found(cws_handler_config_s *config) {
	return cws_handler_error(config, HTTP_NOT_FOUND, "The requested resource was not found.");
}

static cws_response_s *cws_handler_not_implemented(cws_handler_config_s *config) {
	return cws_handler_error(config, HTTP_NOT_IMPLEMENTED, "Method not implemented.");
}

cws_response_s *cws_handler_static_file(cws_request_s *request, cws_handler_config_s *config) {
	if (!request || !config) {
		return cws_handler_error(config, HTTP_INTERNAL_ERROR, "Invalid request or configuration");
	}

	if (request->method != HTTP_GET && request->method != HTTP_HEAD) {
		return cws_handler_not_implemented(config);
	}

	string_s *filepath = resolve_file_path(string_cstr(request->path), config);
	if (!filepath) {
		return cws_handler_not_found(config);
	}

	const char *path = string_cstr(filepath);
	if (!file_exists(path)) {
		string_free(filepath);
		return cws_handler_not_found(config);
	}

	/* Allocate a response object */
	cws_response_s *response = cws_response_new(HTTP_OK);
	if (!response) {
		string_free(filepath);
		return cws_handler_error(config, HTTP_INTERNAL_ERROR, "Failed to create response");
	}

	/* Retrieve Connection header and set it in the response */
	char *conn = cws_request_get_header(request, "Connection");
	if (conn[0] != '\0') {
		cws_response_set_header(response, "Connection", conn);
	}
	free(conn);

	cws_response_set_body_file(response, path);

	/* HEAD: send headers only, release the file descriptor */
	if (request->method == HTTP_HEAD) {
		if (response->body_file) {
			fclose(response->body_file);
			response->body_file = NULL;
		}
		response->body_type = RESPONSE_BODY_NONE;
	}

	cws_log_debug("Serving file: %s (%zu bytes)", path, response->content_length);
	string_free(filepath);

	return response;
}
