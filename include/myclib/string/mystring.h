#ifndef MYCLIB_STRING_H
#define MYCLIB_STRING_H

#include <pthread.h>
#include <stddef.h>

/**
 * @brief Thread-safe dynamic string structure
 */
typedef struct mcl_string_t {
	size_t size;		  /**< Current length of the string (excluding null terminator) */
	size_t capacity;	  /**< Allocated capacity */
	char *data;			  /**< Pointer to string data */
	pthread_mutex_t lock; /**< Mutex for thread safety */
} mcl_string;

/**
 * @brief Create a new string initialized with given text
 *
 * @param text Initial text (cannot be NULL)
 * @param initial_capacity Initial buffer capacity; pass -1 to auto-calculate
 * @return Pointer to new string, or NULL on failure
 *
 * @note Caller must free the string with mcl_string_free()
 */
mcl_string *mcl_string_new(const char *text, long initial_capacity);

/**
 * @brief Append text to an existing string
 *
 * @param string The string to append to
 * @param text Text to append (can be empty)
 * @return 0 on success, -1 on failure
 *
 * @note Original string remains unchanged if append fails
 */
int mcl_string_append(mcl_string *string, const char *text);

/**
 * @brief Free a string and its resources
 *
 * @param string String to free (NULL is safe)
 */
void mcl_string_free(mcl_string *string);

/**
 * @brief Get the current length of the string
 *
 * @param string String to query
 * @return Length of string, or 0 if NULL
 */
size_t mcl_string_length(mcl_string *string);

/**
 * @brief Get the current capacity of the string buffer
 *
 * @param string String to query
 * @return Capacity, or 0 if NULL
 */
size_t mcl_string_capacity(mcl_string *string);

/**
 * @brief Get a read-only C-string pointer
 *
 * @param string String to access
 * @return Null-terminated string pointer, or "" if NULL
 *
 * @warning Do not modify returned pointer.
 *          Pointer may become invalid after string modifications.
 */
const char *mcl_string_cstr(mcl_string *string);

#endif /* MYCLIB_STRING_H */
