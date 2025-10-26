#include "utils/error.h"

#include <stddef.h>

/* @TODO: complete this array */
static cws_error_s errors[] = {{CWS_SERVER_OK, "No error found"}};

char *cws_error_str(cws_server_ret code) {
	for (unsigned long i = 0; i < ARR_SIZE(errors); ++i) {
		if (errors[i].code == code) {
			return errors[i].str;
		}
	}

	return NULL;
}
