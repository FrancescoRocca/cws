#include "http/mime.h"

#include <stdio.h>
#include <string.h>

#include "http/request.h"

static mimetype mimetypes[] = {
	{"html", "text/html"}, {"css", "text/css"},	 {"js", "application/javascript"},
	{"jpg", "image/jpeg"}, {"png", "image/png"}, {"ico", "image/x-icon"}};

int http_get_content_type(const char *location_path, char *content_type) {
	/* Find last occurrence of a string */
	char *ptr = strrchr(location_path, '.');
	if (ptr == NULL) {
		return -1;
	}
	ptr += 1;

	for (size_t i = 0; i < ARR_SIZE(mimetypes); ++i) {
		if (!strcmp(ptr, mimetypes[i].ext)) {
			snprintf(content_type, CWS_HTTP_CONTENT_TYPE - 1, "%s", mimetypes[i].type);
			return 0;
		}
	}

	snprintf(content_type, CWS_HTTP_CONTENT_TYPE - 1, "%s", "Content-Type not supported");

	return 0;
}
