#ifndef CWS_HASH_UTILS_H
#define CWS_HASH_UTILS_H

unsigned int my_str_hash_fn(const void *key);

bool my_str_equal_fn(const void *a, const void *b);

void my_str_free_fn(void *value);

unsigned int my_int_hash_fn(const void *key);

bool my_int_equal_fn(const void *a, const void *b);

void my_int_free_key_fn(void *key);

#endif
