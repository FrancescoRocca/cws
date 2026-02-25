#include "http/handler.h"
#include "utils/debug.h"
#include <myclib/mystring.h>
#include <string.h>
#include <sys/stat.h>

/* Sanitize and resolve file path */
static string_s *resolve_file_path(const char *url_path, cws_handler_config_s *config) {
	string_s *full_path = string_new(config->root, 256);

	if (strcmp(url_path, "/") == 0) {
		string_append(full_path, "/");
		/* Use vhost index file */
		string_append(full_path, "index.html");
		return full_path;
	}

	string_append(full_path, url_path);

	return full_path;
}

static bool file_exists(const char *filepath) {
	struct stat st;
	return stat(filepath, &st) == 0 && S_ISREG(st.st_mode);
}

static cws_response_s *cws_handler_not_found(void) {
	return cws_response_error(HTTP_NOT_FOUND, "The requested resource was not found.");
}

static cws_response_s *cws_handler_not_implemented(void) {
	return cws_response_error(HTTP_NOT_IMPLEMENTED, "Method not implemented.");
}

cws_response_s *cws_handler_static_file(cws_request_s *request, cws_handler_config_s *config) {
	if (!request || !config) {
		return cws_response_error(HTTP_INTERNAL_ERROR, "Invalid request or configuration");
	}

	if (request->method != HTTP_GET) {
		return cws_handler_not_implemented();
	}

	string_s *filepath = resolve_file_path(string_cstr(request->path), config);
	const char *path = string_cstr(filepath);

	if (!file_exists(path)) {
		string_free(filepath);
		return cws_handler_not_found();
	}

	cws_response_s *response = cws_response_new(HTTP_OK);
	if (!response) {
		string_free(filepath);
		return cws_response_error(HTTP_INTERNAL_ERROR, "Failed to create response");
	}

	cws_response_set_body_file(response, path);
	cws_log_debug("Serving file: %s (%zu bytes)", path, response->content_length);
	string_free(filepath);

	return response;
}
