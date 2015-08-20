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
 * Uses GCC attributes to cleanup strings after leaving the scope of the function.
 * Used as a "feature", it's available without SU_String, and requires GCC, but
 * it is a nice enough feature. To use it, declare a string like so:
 * 
 * String string TEMP = "Example String";
 * 
 * When the scope of the variable is left, it will call the cleanup function,
 * SU_String_Destroy, and handle everything for you.
 */
#define TEMP __attribute__ ((__cleanup__(SU_String_destroy)))

#include <stdlib.h> /* Can't malloc without it */
#include <string.h> /* Standard libc library. */
#include <stdio.h> /* ??? */
#include <stdbool.h>
#include <stdarg.h> /* For variadic function concat_all */
#include <assert.h> /* Asserts NULL parameters */
#include <ctype.h> /* To trim strings and check isspace(...). */
#include <MU_Logger.h>
#include <MU_Arg_Check.h>
#include <MU_Flags.h>


bool SU_String_contains(const String str, const String substr, size_t len, bool ignore_case);

String SU_String_lower(String str, size_t len);

String SU_String_upper(String str, size_t len);

char SU_String_char_at(const String str, unsigned int index);

bool SU_String_equal(const String string_one, const String string_two, size_t len, bool ignore_case);

String *SU_String_split(const String str, const String delimiter, size_t len, size_t *size);

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

String SU_String_reverse(String str, size_t len);

String SU_String_join(const String arr[], const String delimiter, size_t size);

String SU_String_replace(String str, char old_char, char new_char, size_t len, bool ignore_case);

bool SU_String_starts_with(const String str, const String find, bool ignore_case);

bool SU_String_ends_with(const String str, const String find, bool ignore_case);

String SU_String_substring(const String str, unsigned int offset, unsigned int end);

String SU_String_trim(String *string_ptr, size_t len);

int SU_String_index_of(const String str, const String substr, size_t len, bool ignore_case);

unsigned int SU_String_count(const String str, const String substr, size_t len, bool ignore_case);

String SU_String_between(const String str, const String start, const String end, size_t len, bool ignore_case);

void SU_String_destroy(String *string_ptr);

#endif	/* SU_String_H */

