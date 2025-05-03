#include "utils/hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/colors.h"

int my_hash_fn(void *key) {
	char *key_str = (char *)key;
	size_t key_len = strlen(key_str);

	int total = 0;

	for (size_t i = 0; i < key_len; ++i) {
		total += (int)key_str[i];
	}

	return total % 2069;
}

bool my_equal_fn(void *a, void *b) {
	if (strcmp((char *)a, (char *)b) == 0) {
		return true;
	}

	return false;
}

void my_free_fn(void *value) { free(value); }

int main(void) {
	bool ret;
	cws_hashmap *str_hashmap = cws_hm_init(my_hash_fn, my_equal_fn, my_free_fn, my_free_fn);

	char *key = strdup("test1");
	char *value = strdup("value1");

	if (key == NULL || value == NULL) {
		CWS_LOG_ERROR("strdup()");

		return 1;
	}

	/* Add a new key-value */
	ret = cws_hm_set(str_hashmap, (void *)key, (void *)value);
	if (!ret) {
		CWS_LOG_WARNING("Unable to set %s:%s", key, value);
	}

	/* Get the added key-value */
	cws_bucket *bucket = cws_hm_get(str_hashmap, key);
	CWS_LOG_DEBUG("Set %s:%s", (char *)bucket->key, (char *)bucket->value);

	/* Update the value */
	char *another_value = strdup("another value1");
	ret = cws_hm_set(str_hashmap, (void *)key, (void *)another_value);
	if (!ret) {
		CWS_LOG_WARNING("Unable to set %s:%s", key, another_value);
	}

	bucket = cws_hm_get(str_hashmap, key);
	CWS_LOG_DEBUG("Set %s:%s", (char *)bucket->key, (char *)bucket->value);

	/* Remove the key-value */
	ret = cws_hm_remove(str_hashmap, (void *)key);
	if (ret) {
		CWS_LOG_DEBUG("test1 removed");
	}
	/* Can't use key, it has been freed */
	bucket = cws_hm_get(str_hashmap, (void *)"test1");
	if (bucket == NULL) {
		CWS_LOG_DEBUG("test1 not found");
	}

	cws_hm_free(str_hashmap);

	return 0;
}
