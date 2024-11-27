#include "http/http.h"

#include <stdio.h> /* Debug */
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "utils/colors.h"

http_t *http_parse(char *request_str) {
	http_t *request = malloc(sizeof(http_t));
	fprintf(stdout, YELLOW "[http] REQUEST:\n%s\n" RESET, request_str);

	/* Parse HTTP method */
	char *pch = strtok(request_str, " ");
	printf("[http] method: %s\n", pch);
	http_parse_method(request, pch);

	/* Parse location */
	pch = strtok(NULL, " ");
	printf("[http] location: %s\n", pch);
	strncpy(request->location, pch, LOCATION_LEN);

	/* Parse location path */
	/* TODO: fix warnings */
	if (strcmp(request->location, "/") == 0) {
		snprintf(request->location_path, LOCATION_LEN, "%s/index.html", WWW);
	} else {
		snprintf(request->location_path, LOCATION_LEN, "%s%s", WWW, request->location);
	}
	fprintf(stdout, "[http] location path: %s\n", request->location_path);

	/* Parse HTTP version */
	pch = strtok(NULL, " \r\n");
	printf("[http] version: %s\n", pch);
	strncpy(request->http_version, pch, HTTP_VERSION_LEN);

	/* Parse other stuff... */

	return request;
}

void http_parse_method(http_t *request, const char *method) {
	if (request == NULL) {
		return;
	}

	if (strcmp(method, "GET") == 0) {
		request->method = GET;
	}
	if (strcmp(method, "POST") == 0) {
		request->method = POST;
	}
}

void http_send_response(http_t *request, int sockfd) {
	FILE *file = fopen(request->location_path, "r");
	if (file == NULL) {
		/* 404 */
		/* TODO: improve error handling */
		char response[1024] =
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: 216\r\n"
			"\r\n"
			"<html>\n"
			"<head>\n"
			"  <title>Resource Not Found</title>\n"
			"</head>\n"
			"<body>\n"
			"<p>Resource not found.</p>\n"
			"</body>\n"
			"</html>";
		send(sockfd, response, 1024, 0);
		return;
	}

	char content_type[1024];
	http_get_content_type(request, content_type);

	/* Don't care about numbers, they are random */
	char line[1024] = {0};
	char html_code[32000] = {0};
	char response[65535] = {0};

	while (fgets(line, 1024, file)) {
		strncat(html_code, line, 32000);
	}
	fclose(file);

	const size_t content_length = strlen(html_code);
	snprintf(response, sizeof response,
			 "%s 200 OK\r\n"
			 "Content-Type: %s\r\n"
			 "Content-Length: %zu\r\n"
			 "Connection: close\r\n"
			 "\r\n"
			 "%s",
			 request->http_version, content_type, content_length, html_code);

	send(sockfd, response, strlen(response), 0);
}

void http_get_content_type(http_t *request, char *content_type) {
	char *ptr = strrchr(request->location_path, '.');
	/* TODO: improve content_type (used to test) */
	snprintf(content_type, 1024, "text/%s", ptr + 1);
}

void http_free(http_t *request) { free(request); }
