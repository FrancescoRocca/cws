#include "http/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "utils/colors.h"
#include "utils/utils.h"

cws_http *cws_http_parse(char *request_str, int sockfd) {
	cws_http *request = malloc(sizeof(cws_http));
	if (request == NULL) {
		return NULL;
	}
	memset(request, 0, sizeof(cws_http));

	/* Insert socket file descriptor */
	request->sockfd = sockfd;

	/* Parse HTTP method */
	char *pch = strtok(request_str, " ");
	if (pch == NULL) {
		return NULL;
	}
	CWS_LOG_DEBUG("[http] method: %s", pch);
	cws_http_parse_method(request, pch);

	/* Parse location */
	pch = strtok(NULL, " ");
	if (pch == NULL) {
		return NULL;
	}
	CWS_LOG_DEBUG("[http] location: %s", pch);
	strncpy(request->location, pch, CWS_HTTP_LOCATION_LEN);

	/* Adjust location path */
	if (strcmp(request->location, "/") == 0) {
		snprintf(request->location_path, CWS_HTTP_LOCATION_PATH_LEN, "%s/index.html", CWS_WWW);
	} else {
		snprintf(request->location_path, CWS_HTTP_LOCATION_PATH_LEN, "%s%s", CWS_WWW, request->location);
	}
	CWS_LOG_DEBUG("[http] location path: %s", request->location_path);

	/* Parse HTTP version */
	pch = strtok(NULL, " \r\n");
	if (pch == NULL) {
		return NULL;
	}
	CWS_LOG_DEBUG("[http] version: %s", pch);
	strncpy(request->http_version, pch, CWS_HTTP_VERSION_LEN);

	/* Parse headers until a \r\n */
	request->headers = cws_hm_init(my_str_hash_fn, my_str_equal_fn, my_str_free_fn, my_str_free_fn);
	char *header_colon;
	while (pch) {
		/* Get header line */
		pch = strtok(NULL, "\r\n");
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

		cws_hm_set(request->headers, hkey_dup, hvalue_dup);
	}

	/* Parse body */

	return request;
}

void cws_http_parse_method(cws_http *request, const char *method) {
	if (strcmp(method, "GET") == 0) {
		request->method = CWS_HTTP_GET;
		return;
	}
	if (strcmp(method, "POST") == 0) {
		request->method = CWS_HTTP_POST;
		return;
	}

	cws_http_send_not_implemented(request);
}

void cws_http_send_response(cws_http *request) {
	FILE *file = fopen(request->location_path, "rb");
	if (file == NULL) {
		/* 404 */
		cws_http_send_not_found(request);
		return;
	}

	/* Retrieve correct Content-Type */
	char content_type[1024];
	cws_http_get_content_type(request, content_type);

	/* Retrieve file size */
	fseek(file, 0, SEEK_END);
	const size_t content_length = ftell(file); /* Returns the read bytes (we're at the end, so the file size) */
	rewind(file);

	/* Retrieve file data */
	char *file_data = malloc(content_length);
	if (file_data == NULL) {
		fclose(file);
		CWS_LOG_ERROR("Unable to allocate file data");
		return;
	}

	/* Read 1 byte until content_length from file and put in file_data */
	fread(file_data, 1, content_length, file);
	fclose(file);

	char response_header[2048];
	snprintf(response_header, sizeof response_header,
			 "%s 200 OK\r\n"
			 "Content-Type: %s\r\n"
			 "Content-Length: %zu\r\n"
			 "Connection: close\r\n"
			 "\r\n",
			 request->http_version, content_type, content_length);

	send(request->sockfd, response_header, strlen(response_header), 0);
	send(request->sockfd, file_data, content_length, 0);

	free(file_data);
}

void cws_http_get_content_type(cws_http *request, char *content_type) {
	char *ptr = strrchr(request->location_path, '.');
	if (ptr == NULL) {
		cws_http_send_not_found(request);
		return;
	}
	ptr += 1;

	char ct[32];

	/* TODO: Improve content_type (used to test) */
	if (strcmp(ptr, "html") == 0 || strcmp(ptr, "css") == 0 || strcmp(ptr, "javascript") == 0) {
		strncpy(ct, "text", sizeof ct);
	}

	if (strcmp(ptr, "jpg") == 0 || strcmp(ptr, "png") == 0) {
		strncpy(ct, "image", sizeof ct);
	}

	snprintf(content_type, 1024, "%s/%s", ct, ptr);
}

void cws_http_send_not_implemented(cws_http *request) {
	const char response[1024] =
		"HTTP/1.1 501 Not Implemented\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 216\r\n"
		"\r\n"
		"<html>\n"
		"<head>\n"
		"  <title>501 Not Implemented</title>\n"
		"</head>\n"
		"<body>\n"
		"<p>501 Not Implemented</p>\n"
		"</body>\n"
		"</html>";
	const size_t response_len = strlen(response);
	send(request->sockfd, response, response_len, 0);
}

void cws_http_send_not_found(cws_http *request) {
	const char html_body[] =
		"<html>\n"
		"<head>\n"
		"  <title>404 Not Found</title>\n"
		"</head>\n"
		"<body>\n"
		"<p>404 Not Found.</p>\n"
		"</body>\n"
		"</html>";
	int html_body_len = strlen(html_body);

	char response[1024];
	int response_len = snprintf(response, sizeof(response),
								"HTTP/1.1 404 Not Found\r\n"
								"Content-Type: text/html\r\n"
								"Content-Length: %d\r\n"
								"\r\n"
								"%s",
								html_body_len, html_body);

	send(request->sockfd, response, response_len, 0);
}

void cws_http_free(cws_http *request) {
	cws_hm_free(request->headers);
	free(request);
}
