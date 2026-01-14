#ifndef CWS_MIME_H
#define CWS_MIME_H

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct mimetype {
	const char *ext;
	const char *type;
} mimetype;

/*
 * Retrieve Content Type
 */
int cws_mime_get_ct(const char *location_path, char *content_type);

#endif
