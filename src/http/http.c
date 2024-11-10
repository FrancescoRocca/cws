#include "http/http.h"

#include "utils/colors.h"

http_t *http_parse(char *request_str) {
	http_t *request = malloc(sizeof(http_t));
	fprintf(stdout, YELLOW "[http] REQUEST:\n%s\n" RESET, request_str);

	/* Parse HTTP method */
	char *pch = strtok(request_str, " ");
	printf("%s\n", pch);
	http_parse_method(request, pch);

	/* Parse location */
	pch = strtok(NULL, " ");
	printf("%s\n", pch);
	strncpy(request->location, pch, LOCATION_LEN);

	/* Parse HTTP version */
	pch = strtok(NULL, " \r\n");
	printf("%s\n", pch);
	strncpy(request->http_version, pch, HTTP_VERSION_LEN);

	/* Parse other stuff... */

	return request;
}

void http_parse_method(http_t *request, const char *method) {
	if (strcmp(method, "GET") == 0) {
		request->method = GET;
	}
	if (strcmp(method, "POST") == 0) {
		request->method = POST;
	}
}

void http_send_response(http_t *request) { /* TODO */ }

void http_free(http_t *request) { free(request); }
