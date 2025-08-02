#include "mystring.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

mcl_string *mcl_string_new(const char *text, long initial_capacity) {
	if (!text) {
		return NULL;
	}

	/* Allocate string struct */
	mcl_string *str = malloc(sizeof(mcl_string));
	if (!str) {
		return NULL;
	}

	/* Calculate size and capacity */
	str->size = strlen(text);
	size_t capacity = initial_capacity;

	if (capacity != -1 && capacity - 1 < str->size) {
		return NULL;
	}

	if (capacity == -1) {
		capacity = (unsigned long)pow(2, (unsigned)log2(str->size) + 1);
	}

	str->capacity = capacity;

	/* Allocate data buffer */
	str->data = malloc(sizeof(char) * str->capacity);
	if (!str->data) {
		free(str);

		return NULL;
	}

	/* Copy the text and ensure null termination */
	memset(str->data, 0, str->capacity);
	memcpy(str->data, text, str->size);
	str->data[str->size] = '\0';

	return str;
}

int mcl_string_append(mcl_string *string, const char *text) {
	if (!string || !text) {
		return -1;
	}

	/* Handle empty case */
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return 0;
	}

	size_t new_size = text_len + string->size;

	/* Check if we need to resize */
	if (new_size + 1 > string->capacity) {
		size_t new_capacity = (unsigned long)pow(2, (unsigned)log2(new_size) + 1);
		/* Reallocate the buffer */
		void *new_data = realloc(string->data, sizeof(char) * new_capacity);
		if (!new_data) {
			return -1;
		}

		string->data = new_data;
		string->capacity = new_capacity;

		/* Init to 0 the new capacity */
		memset(string->data + string->size, 0, string->capacity - string->size);
	}

	/* Append text */
	memcpy(string->data + string->size, text, text_len);
	string->size = new_size;
	string->data[string->size] = '\0';

	return 0;
}

void mcl_string_free(mcl_string *string) {
	if (!string) {
		return;
	}

	if (string->data) {
		free(string->data);
	}

	free(string);
}

size_t mcl_string_length(const mcl_string *string) {
	if (!string) {
		return 0;
	}

	return string->size;
}

size_t mcl_string_capacity(const mcl_string *string) {
	if (!string) {
		return 0;
	}

	return string->capacity;
}

const char *mcl_string_cstr(const mcl_string *string) {
	if (!string || !string->data) {
		return "";
	}

	return string->data;
}
