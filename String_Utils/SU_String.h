/*
 * @Author: Louis Jenkins
 * @Version: 2.0
 * 
 * A basic, yet very powerful and conventional string manipulations library.
 * Supports ASCII strings only, and some functions support the use of non NULL-terminated functions.
 *
 * From simple string reversal or splitting and joining a string based on a delimiter, or even dynamic concatenation of strings,
 * is all included in this library. This library fixes and improves upon the standard libc and glibc library
 * by adding functionality that is sorely missing, in an efficient manner.
 *
 * There is also a convenience typedef for cstrings, String, which abstracts the need to use pointers.
 * Lastly, there is a convenience macro that can be used to handle memory management of non-constant strings, 
 * TEMP, which utilitizes the GCC or Clang's compiler attributes.
 * 
 */

#ifndef SU_String_H
#define	SU_String_H

/**
* Convenience typedef of char *.
*/
typedef char *String;

/**
 * Uses compiler attributes to cleanup strings after leaving the scope of the function.
 * Used as a "feature", it's available without SU_String, and requires GCC and Clang, but
 * it is a nice enough feature. To use it, declare a string like so:
 * 
 * String string TEMP = "Example String";
 * 
 * When the scope of the variable is left, it will call the cleanup function,
 * SU_String_Destroy, and handle everything for you.
 */
#define TEMP __attribute__ ((__cleanup__(SU_String_destroy)))

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h> 
#include <MU_Logger.h>
#include <MU_Arg_Check.h>
<<<<<<< HEAD
#include <MU_Flags.h>
=======
>>>>>>> development

/**
 * Scans the string for the passed substring up to the passed length. If 0
 * is passed, strlen is used to determine the string's length, hence a non NULL-terminated
 * string can be safely used if len is not 0. String can be READ-ONLY.
 * @param str String to search.
 * @param substr Substring to find. Must be NULL-terminated.
 * @param len How much of the string to search. If 0, strlen is used.
 * @param ignore_case If true, case insensitive, else case sensitive.
 * @return true if it contains the substr, or false if it doesn't or if str or substr is NULL.
 */
bool SU_String_contains(const String str, const String substr, size_t len, bool ignore_case);

/**
 * Converts a string to lowercase, up to the passed length. If 0
 * is passed, strlen is used to determine the string's length, hence a non NULL-terminated
 * string can be safely used if len is not 0. The string must NOT be READ-ONLY, as it modifies the string passed.
 * @param str String to lower.
 * @param len Amount of string to lower. If 0, strlen is used.
 * @return The beginning of str, or NULL if str is NULL.
 */
String SU_String_lower(String str, size_t len);

/**
 * Converts the string to uppercase, up to the passed length. If 0
 * is passed, strlen is used to determine the string's length, hence a non NULL-terminated
 * string can be safely used if len is not 0. The string must NOT be READ-ONLY, as it modifies
 * the string passed.
 * @param str String to upper.
 * @param len Amount of string to upper. If 0, strlen is used.
 * @return The beginning of str, or NULL if str is NULL.
 */
String SU_String_upper(String str, size_t len);

/**
 * Obtains the character at the given index, with bounds checking. The string MUST be
 * NULL-terminated, but CAN be READ-ONLY. If the index is out of bounds of the string, it will return the NULL-terminator '\0'.
 * @param str The string.
 * @param index The index, if out of bounds it defaults to the index of the NULL-terminator.
 * @return The character at the index, or '\0' if it is out of bounds or if str is NULL.
 */
char SU_String_char_at(const String str, unsigned int index);

/**
 * Compares two string to see if they are equal, up to len. If len is 0, strlen is used to determine the string's length,
 * hence theoretically a non NULL-terminated string can be safely used, if and only if the length does not exceed the valid bounds of
 * both strings.
 * @param string_one First string to compare.
 * @param string_two Second string to compare.
 * @param len The amount to compare up to, if 0 strlen of the first string is used.
 * @param ignore_case If true, comparison is case insensitive.
 * @return true if they are equal, false if they are not or if either string_one or string_two are NULL.
 */
bool SU_String_equal(const String string_one, const String string_two, size_t len, bool ignore_case);

/**
 * Splits the given string into many strings according to the delimiter. The pointer to a size_t variable is
 * used to return additional information, I.E the size of the array. If len is 0, strlen is used to determine
 * the length of the string to split up to, hence a non NULL-terminated string may safely be used.
 * @param str The string
 * @param delimiter Delimiter to split by.
 * @param len The length of the string.
 * @param size Used to return the size of the array of strings.
 * @return An array of strings with updated to indicate it's size, or NULL if either str or delimiter are NULL or if the delimiter could not be found.
 */
String *SU_String_split(const String str, const String delimiter, size_t len, size_t *size);

/**
 * Concatenates all strings passed, with the delimiter inserted in between each string, into one string.
 * The result is placed inside str_storage_ptr. Each string MUST be NULL-terminated.
 * @param str_storage_ptr Pointer to a string to hold the result.
 * @param delim Delimiter to insert between each string.
 * @param ... List of strings to be concatenated.
 */
#define SU_STRING_CONCAT_ALL(str_storage_ptr, delim, ...) do { \
	const int tok_len = 2; \
	int arg_size = (sizeof((String[]){__VA_ARGS__})/sizeof(String)); \
	char format[BUFSIZ + 1]; \
	/* \
		Calculate argument length by size of delimiter if non-NUll, then strlen("%s"), which is 2. \
	*/ \
	int arg_len = (delim ? strlen(delim) : 0) + tok_len; \
	int i = 0, str_left = BUFSIZ; \
	/* \
		We need to generate the appropriate format based on the number of arguments passed! \
	*/ \
	for(; i < arg_size; i++){ \
		if(i == 0){ \
			snprintf(format, str_left, "%s", "%s"); \
			str_left -= tok_len; \
			continue; \
		} \
		/* \
			If there isn't enough space left to hold the format, break. \
		*/ \
		if(str_left < arg_len) break; \
		snprintf(format, str_left, "%s%s%s", format, delim ? delim : "", "%s"); \
		str_left -= arg_len; \
	} \
	asprintf(str_storage_ptr, format, __VA_ARGS__); \
} while(0)

/**
 * Reverses the string up to the passed length. If len is 0, strlen is used. If len is specified, 
 * the string does not need to be NULL-terminated. String must NOT be READ-ONLY.
 * @param str The string.
 * @param len Length of the string to reverse up to. If len is 0, strlen is used to determine string length.
 * @return The start of the reversed string.
 */
String SU_String_reverse(String str, size_t len);

/**
 * Joins the array of strings together, with the passed delimiter in between, into
 * one string. All strings in arr must be NULL-terminated. 
 * @param arr Array of strings.
 * @param delimiter Delimiter to insert between each string.
 * @param size Size of the array of strings.
 * @return The combined string.
 */
String SU_String_join(const String arr[], const String delimiter, size_t size);

/**
 * Scans the string for old_char and replaces it with new_chat, up to the passed length. 
 * If len is 0, strlen is used to determine string length. str does not need to be NULL-terminated
 * but it does have to be READ-ONLY.
 * @param str The string
 * @param old_char Old character to search for.
 * @param new_char New character to replace old_char with.
 * @param len Length to read up to. Strlen is used if len is 0.
 * @param ignore_case Case insensitive comparison is used if true.
 * @return The beginning of str, NULL if str is NULL.
 */
String SU_String_replace(String str, char old_char, char new_char, size_t len, bool ignore_case);

/**
 * Determines if the string passed starts with find. Both must be NULL-terminated.
 * @param str The string
 * @param find String to find
 * @param ignore_case Case insensitive if true.
 * @return  true if it does, false if it does not or if str or find are NULL.
 */
bool SU_String_starts_with(const String str, const String find, bool ignore_case);

/**
 * Determines if the string passed ends with find. Both must be NULL-terminated.
 * @param str The string
 * @param find String to find
 * @param ignore_case Case insensitive if true.
 * @return  true if it does, false if it does not or if str or find are NULL.
 */
bool SU_String_ends_with(const String str, const String find, bool ignore_case);

/**
 * Creates a substring at the given offset up to end. str need not be NULL-terminated,
 * but offset and end must be within it's bounds.
 * @param str The string
 * @param offset Offset to start at
 * @param end The end of the string.
 * @return The substring or NULL if str is NULL.
 */
String SU_String_substring(const String str, unsigned int offset, unsigned int end);

/**
 * Trims the string of leading and trailing white spaces. If len is 0, strlen is used
 * to determine the string length. The string pointed to by string_ptr does not need to be
 * NULL-terminated nor READ-ONLY.
 * @param string_ptr Pointer to a string to store the result, as well as the original string.
 * @param len Length fo the string to trim up to.
 * @return The new string.
 */
String SU_String_trim(String *string_ptr, size_t len);

/**
 * Obtains the beginning index of the substr in str if it exists, up to len. If len is 0, strlen is used
 * to determine the string length. The string does not need to be NULL-terminated if length is specified.
 * @param str The string.
 * @param substr The substring to search for.
 * @param len The length of the string to search up to.
 * @param ignore_case if true, search is case insensitive.
 * @return The index at the beginning of the substr.
 */
int SU_String_index_of(const String str, const String substr, size_t len, bool ignore_case);

/**
 * Counts the occurences of the substr in the given str up to len (strlen if len == 0).
 * @param str The string
 * @param substr Substring to count.
 * @param len The length of the string to read up to. strlen used if len == 0.
 * @param ignore_case Search is case insensitive if true.
 * @return The occurences of the substr in str.
 */
unsigned int SU_String_count(const String str, const String substr, size_t len, bool ignore_case);

/**
 * Returns the substring between start and end in str, up to len (strlen if len == 0).
 * @param str The string.
 * @param start The first substr to find.
 * @param end The second substr to find.
 * @param len Length of string to search up to.
 * @param ignore_case Search is case insensitive if true.
 * @return The substring between start and end.
 */
String SU_String_between(const String str, const String start, const String end, size_t len, bool ignore_case);

/**
 * Destroys the passed string, used by compiler attribute.
 * @param string_ptr Pointer to the string being destroyed.
 */
void SU_String_destroy(String *string_ptr);

#endif	/* SU_String_H */

