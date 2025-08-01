#include "http/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "utils/colors.h"
#include "utils/utils.h"

cws_http *cws_http_parse(char *request_str, int sockfd, cws_config *config) {
	if (!request_str || !config) {
		return NULL;
	}

	cws_http *request = calloc(1, sizeof(cws_http));
	if (request == NULL) {
		return NULL;
	}
	request->sockfd = sockfd;

	char *request_str_cpy = strdup(request_str);
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
	CWS_LOG_DEBUG("method: %s", pch);

	int ret = cws_http_parse_method(request, pch);
	if (ret < 0) {
		/* Not implemented */
		cws_http_free(request);
		free(request_str_cpy);

		cws_http_send_response(request, CWS_HTTP_NOT_IMPLEMENTED);

		return NULL;
	}

	/* Parse location */
	pch = strtok_r(NULL, " ", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_str_cpy);

		return NULL;
	}
	CWS_LOG_DEBUG("location: %s", pch);
	strncpy(request->location, pch, CWS_HTTP_LOCATION_LEN);

	/* Adjust location path */
	if (strcmp(request->location, "/") == 0) {
		snprintf(request->location_path, CWS_HTTP_LOCATION_PATH_LEN, "%s/index.html", config->www);
	} else {
		size_t location_path_len = strlen(request->location);
		if (location_path_len > CWS_HTTP_LOCATION_PATH_LEN) {
			request->location[location_path_len - 1] = '\0';
		}
		snprintf(request->location_path, CWS_HTTP_LOCATION_PATH_LEN, "%s%s", config->www, request->location);
	}
	CWS_LOG_DEBUG("location path: %s", request->location_path);

	/* Parse HTTP version */
	pch = strtok_r(NULL, " \r\n", &saveptr);
	if (pch == NULL) {
		cws_http_free(request);
		free(request_str_cpy);

		return NULL;
	}
	CWS_LOG_DEBUG("version: %s", pch);
	strncpy(request->http_version, pch, CWS_HTTP_VERSION_LEN);

	/* Parse headers until a \r\n */
	request->headers = mcl_hm_init(my_str_hash_fn, my_str_equal_fn, my_str_free_fn, my_str_free_fn);
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

		/* Header key */
		char *hkey = pch;
		char *hkey_dup = strdup(hkey);
		/* Header value (starting from ": ") */
		char *hvalue = header_colon + 2;
		char *hvalue_dup = strdup(hvalue);

		mcl_hm_set(request->headers, hkey_dup, hvalue_dup);
	}

	/* TODO: Parse body */

	return request;
}

int cws_http_parse_method(cws_http *request, const char *method) {
	if (strcmp(method, "GET") == 0) {
		request->method = CWS_HTTP_GET;
		return 0;
	}

	if (strcmp(method, "POST") == 0) {
		request->method = CWS_HTTP_POST;
		return 0;
	}

	return -1;
}

char *cws_http_status_string(cws_http_status status) {
	switch (status) {
		case CWS_HTTP_OK: {
			return "200 OK";
			break;
		}
		case CWS_HTTP_NOT_FOUND: {
			return "404 Not Found";
			break;
		}
		case CWS_HTTP_NOT_IMPLEMENTED: {
			return "501 Not Implemented";
			break;
		}
	}

	return "?";
}

size_t cws_http_response_builder(char **response, char *http_version, cws_http_status status, char *content_type, char *connection, char *body,
								 size_t body_len_bytes) {
	char *status_code = cws_http_status_string(status);

	size_t header_len = snprintf(NULL, 0,
								 "%s %s\r\n"
								 "Content-Type: %s\r\n"
								 "Content-Length: %ld\r\n"
								 "Connection: %s\r\n"
								 "\r\n",
								 http_version, status_code, content_type, body_len_bytes, connection);

	size_t total_len = header_len + body_len_bytes;

	*response = malloc(total_len);
	if (*response == NULL) {
		return 0;
	}

	snprintf(*response, header_len + 1, "%s %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: %s\r\n\r\n", http_version, status_code, content_type,
			 body_len_bytes, connection);

	memcpy(*response + header_len, body, body_len_bytes);

	return total_len;
}

void cws_http_send_response(cws_http *request, cws_http_status status) {
	switch (status) {
		case CWS_HTTP_OK:
			break;
		case CWS_HTTP_NOT_FOUND: {
			cws_http_send_simple_html(request, CWS_HTTP_NOT_FOUND, "404 Not Found", "Resource not found, 404.");
			break;
		}
		case CWS_HTTP_NOT_IMPLEMENTED: {
			cws_http_send_simple_html(request, CWS_HTTP_NOT_IMPLEMENTED, "501 Not Implemented", "Method not implemented, 501.");
			break;
		}
	}
}

void cws_http_send_resource(cws_http *request) {
	FILE *file = fopen(request->location_path, "rb");
	if (file == NULL) {
		cws_http_send_response(request, CWS_HTTP_NOT_FOUND);
		return;
	}

	/* Retrieve correct Content-Type */
	char content_type[1024];
	int ret = cws_http_get_content_type(request, content_type);
	if (ret < 0) {
		cws_http_send_response(request, CWS_HTTP_NOT_FOUND);
	}

	/* Retrieve file size */
	fseek(file, 0, SEEK_END);
	const size_t content_length = ftell(file);
	rewind(file);

	/* Retrieve file data */
	char *file_data = malloc(content_length);
	if (file_data == NULL) {
		fclose(file);
		CWS_LOG_ERROR("Unable to allocate file data");
		return;
	}

	/* Read 1 byte until content_length from file and put in file_data */
	size_t read_bytes = fread(file_data, 1, content_length, file);
	fclose(file);

	if (read_bytes != content_length) {
		free(file_data);
		CWS_LOG_ERROR("Partial read from file");
		return;
	}

	char conn[32] = "close";
	mcl_bucket *connection = mcl_hm_get(request->headers, "Connection");
	if (connection) {
		strncpy(conn, (char *)connection->value, sizeof(conn));
	}

	char *response = NULL;
	size_t response_len = cws_http_response_builder(&response, "HTTP/1.1", CWS_HTTP_OK, content_type, conn, file_data, content_length);

	size_t bytes_sent = 0;
	do {
		size_t sent = send(request->sockfd, response + bytes_sent, response_len, 0);
		bytes_sent += sent;
	} while (bytes_sent < response_len);

	free(response);
	free(file_data);
}

int cws_http_get_content_type(cws_http *request, char *content_type) {
	char *ptr = strrchr(request->location_path, '.');
	if (ptr == NULL) {
		return -1;
	}
	ptr += 1;

	char ct[32] = {0};

	/* TODO: Improve content_type (used to test) */
	if (strcmp(ptr, "html") == 0 || strcmp(ptr, "css") == 0 || strcmp(ptr, "javascript") == 0) {
		strncpy(ct, "text", sizeof ct);
	}

	if (strcmp(ptr, "jpg") == 0 || strcmp(ptr, "png") == 0) {
		strncpy(ct, "image", sizeof ct);
	}

	snprintf(content_type, 1024, "%s/%s", ct, ptr);

	return 0;
}

void cws_http_send_simple_html(cws_http *request, cws_http_status status, char *title, char *description) {
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
	size_t body_len = strlen(body) * sizeof(char);

	char conn[32] = "close";
	mcl_bucket *connection = mcl_hm_get(request->headers, "Connection");
	if (connection) {
		strncpy(conn, (char *)connection->value, sizeof(conn));
	}

	char *response = NULL;
	size_t response_len = cws_http_response_builder(&response, "HTTP/1.1", status, "text/html", conn, body, body_len);
	send(request->sockfd, response, response_len, 0);
	free(response);
}

void cws_http_free(cws_http *request) {
	mcl_hm_free(request->headers);
	free(request);
}
