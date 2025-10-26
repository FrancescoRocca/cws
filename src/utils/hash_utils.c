#include "utils/hash.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned int my_str_hash_fn(const void *key) {
	char *key_str = (char *)key;
	size_t key_len = strlen(key_str);

	int total = 0;

	for (size_t i = 0; i < key_len; ++i) {
		total += (int)key_str[i];
	}

	return total % 2069;
}

bool my_str_equal_fn(const void *a, const void *b) {
	if (strcmp((char *)a, (char *)b) == 0) {
		return true;
	}

	return false;
}

void my_str_free_fn(void *value) {
	free(value);
}

unsigned int my_int_hash_fn(const void *key) {
	return *(int *)key;
}

bool my_int_equal_fn(const void *a, const void *b) {
	int ai = *(int *)a;
	int bi = *(int *)b;

	if (ai == bi) {
		return true;
	}

	return false;
}

void my_int_free_key_fn(void *key) {
	int fd = *(int *)key;
	close(fd);
	free(key);
}
