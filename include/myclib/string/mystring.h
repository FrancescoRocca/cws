#ifndef MYCLIB_STRING_H
#define MYCLIB_STRING_H

#include <stddef.h>

/**
 * @brief String structure
 */
typedef struct mcl_string_t {
	size_t size;	 /**< Length of the string (excluding null terminator) */
	size_t capacity; /**< Total allocated capacity */
	char *data;		 /**< Pointer to the string data */
} mcl_string;

/**
 * @brief Create a new string
 *
 * @param text The text to initialize from
 * @param initial_capacity The initial capacity, pass -1 to retrieve it from the text
 * @return Pointer to the new string, or NULL on failure
 *
 * @note The caller is responsible for freeing the returned string with mcl_string_free() and to pass the right inital_capacity
 */
mcl_string *mcl_string_new(const char *text, long initial_capacity);

/**
 * @brief Append text to an existing string
 *
 * @param string The string to append to
 * @param text The string to append (can be empty)
 * @return 0 on success, -1 on failure
 *
 * @note If it fails, the original string remains unchanged
 */
int mcl_string_append(mcl_string *string, const char *text);

/**
 * @brief Free a string
 *
 * @param string The string to free
 *
 * @note This function is safe to call with NULL pointers
 */
void mcl_string_free(mcl_string *string);

/**
 * @brief Get the current length of the string
 *
 * @param string The string to query
 * @return The length of the string, or 0 if string is NULL
 */
size_t mcl_string_length(const mcl_string *string);

/**
 * @brief Get the current capacity of the string
 *
 * @param string The string to query
 * @return The capacity of the string buffer, or 0 if string is NULL
 */
size_t mcl_string_capacity(const mcl_string *string);

/**
 * @brief Get a read-only string representation
 *
 * @param string The string to access
 * @return Pointer to null-terminated string data, or empty string "" if string is NULL
 *
 * @warning The returned pointer should not be modified directly and may become
 *          invalid after any modification operation on the string
 */
const char *mcl_string_cstr(const mcl_string *string);

#endif /* MYCLIB_STRING_H */
