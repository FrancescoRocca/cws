#include "http/mime.h"

#include <stdio.h>
#include <string.h>

#include "http/request.h"
#include "utils/error.h"

static mimetype mimetypes[] = {{"html", "text/html"}, {"css", "text/css"},	{"js", "application/javascript"},
							   {"jpg", "image/jpeg"}, {"png", "image/png"}, {"ico", "image/x-icon"}};

int cws_mime_get_ct(const char *location_path, char *content_type) {
	/* Find last occurrence of a string */
	char *ptr = strrchr(location_path, '.');
	if (ptr == NULL) {
		return CWS_CONTENT_TYPE_ERROR;
	}
	ptr += 1;

	for (size_t i = 0; i < ARR_SIZE(mimetypes); ++i) {
		if (!strcmp(ptr, mimetypes[i].ext)) {
			snprintf(content_type, CWS_HTTP_CONTENT_TYPE - 1, "%s", mimetypes[i].type);

			return CWS_OK;
		}
	}

	snprintf(content_type, CWS_HTTP_CONTENT_TYPE - 1, "%s", "Content-Type not supported");

	return CWS_OK;
}
