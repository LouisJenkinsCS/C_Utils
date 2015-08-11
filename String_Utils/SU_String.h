/*
 * @Author: Louis Jenkins
 * @Version: 1.2
 * 
 * SU_String is a string library based off of Java's own String library, except
 * the library works on String over a string object (or struct). It is a lightweight,
 * relatively speedy library which relies upon the libc and glibc. 
 * 
 * SU_String also features passing flags with the '|' operator, for example,
 * SU_IGNORE_CASE|SU_LAST does exactly as it says, ignore case for string comparison,
 * and return the last string that matches the criteria. For default behavior, you
 * pass SU_NONE flag instead. 
 * 
 * Some functions require passing the address of the string (&) for possible modification
 * of the original string, as SU_String never modifies the string passed, by default,
 * so it is safe to use string literals unless passing SU_MODIFY flag.
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

/**
 * Searches for the substring inside of the passed string. The string need not be
 * null terminated if len is passed. 
 * @param str The string to search.
 * @param substr The substring to search for in the passed string.
 * @param len The length of the string and amount of the string to search, strlen is used if len is 0.
 * @param flagss SU_IGNORE_CASE
 * @return true if found, false if not found.
 */
bool SU_String_contains(const String str, const String substr, size_t len, bool ignore_case);

/**
 * Returns a lowercase of the string passed. If SU_MODIFY is passed, it will modify
 * the original string, freeing it. String does not need to be null terminated if length
 * is passed.
 * @param string_ptr Pointer to the String to be manipulated.
 * @param len Length of the string and amount to convert to lowercase, strlen is used if len is 0.
 * @param flags SU_MODIFY
 * @return Lowercase string.
 */
String SU_String_lower(String str, size_t len);

/**
 * Creates a copy of the string and converts it to uppercase. If SU_MODIFY is passed, it
 * will modify the original string, freeing it. String does not need to be null terminated,
 * if length is passed.
 * @param string_ptr Pointer to the String to be manipulated.
 * @param len Length of the string and amount to convert to uppercase, strlen is used if len is 0.
 * @param flags SU_MODIFY
 * @return Uppercase string.
 */
String SU_String_upper(String str, size_t len);

/**
 * Returns the character at the given index, and if it is out of bounds, it will return
 * the NULL terminator character, '\0', instead. String must be null terminated!
 * @param string String to retrieve character from.
 * @param index Index of the char.
 * @return The character at the index of the string, or NULL terminator upon overflow.
 */
char SU_String_char_at(const String str, unsigned int index);

/**
 * Checks to see if two strings are equal. It will compare up to the length passed if declared, otherwise strlen is used.
 * @param string_one First string to compare.
 * @param string_two Second string to compare.
 * @param len The length to compare up to, if len is 0 then strlen will be used.
 * @param flags SU_IGNORE_CASE
 * @return true if equal, false if not.
 */
bool SU_String_equal(const String string_one, const String string_two, size_t len, bool ignore_case);

/**
 * Splits a string into an array of strings based on delimiter passed. size should be non-NULL,
 * I.E (&size), and is used to record the new size of the array returned. If len is passed, it will
 * only split up to the passed length, otherwise it is assumed the string is NULL terminated and strlen is used.
 * @param string String to be split.
 * @param len The length to split up to, if len is 0, then strlen is used.
 * @param delimiter Delimiter to look for when splitting.
 * @param size Records size of the string array.
 * @return The array of strings, plus sets size to record the size of the array.
 */
String *SU_String_split(const String str, const String delimiter, size_t len, size_t *size);

#define SU_STRING_CONCAT_ALL(flags, str_storage_ptr, delim, ...) do { \
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
		snprintf(format, arg_len, "%s%s%s", format, delim ? delim : "", "%s"); \
		str_left -= arg_len; \
	} \
	asprintf(str_storage_ptr, format, __VA_ARGS__); \
} while(0)

/**
 * Reverses the given string. A non-NULL terminated string may be used if len is specified.
. * @param string Pointer to the String to be operated on.
 * @param len Length of the string to reverse, strlen is used if len is 0.
 * @param flags SU_MODIFY
 * @return The reversed string.
 */
String SU_String_reverse(String str, size_t len);

/**
 * Joins an array of strings together into one with the delimiter prepended to each string after the first.
 * All strings must be null terminated. 
 * @param array_of_strings The array of strings to be joined.
 * @param delimiter Delimiter to be prepended to each string after the first
 * @param size The size of the array
 * @return The new string that was joined.
 */
String SU_String_join(const String arr[], const String delimiter, size_t size);

/**
 * Replaces all of one character in a string with another character. A non-NULL terminated string
 * may be passed if len is specified.
 * @param string Pointer to the The string the characters are to be replaced
 * @param old_char The characters to be found
 * @param new_char The characters that will replace the old_char
 * @param len Length of the string to search. If len is 0, strlen is used.
 * @param flags SU_IGNORE_CASE | SU_MODIFY
 * @return The new string with replaced characters.
 */
String SU_String_replace(String str, char old_char, char new_char, size_t len, bool ignore_case);

/**
 * Checks to see if a string starts with a substring
 * @param string he string to check.
 * @param find The substring to check for.
 * @param flags SU_IGNORE_CASE
 * @return 1 if true, 0 if false.
 */
bool SU_String_starts_with(const String str, const String find, bool ignore_case);

/**
 * Checks to see if a string ends with a substring.
 * @param string The string to check.
 * @param find The substring to check for.
 * @param flags SU_IGNORE_CASE
 * @return  1 if true, 0 if false.
 */
bool SU_String_ends_with(const String str, const String find, bool ignore_case);

/**
 * Returns a substring of the string.
 * @param string_ptr Pointer to the String to get a substring of.
 * @param offset The beginning index.
 * @param end The end index. If end is 0, strlen is used.
 * @param flags SU_NONE | SU_MODIFY
 * @return The substring of the string.
 */
String SU_String_substring(const String str, unsigned int offset, unsigned int end);

/**
 * Trims the string of all leading and trailing spaces.
 * @param string Pointer to the String to be trimmed.
 * @param flags SU_NONE | SU_MODIFY
 * @return Trimmed string.
 */
String SU_String_trim(String *string_ptr, size_t len);

/**
 * Finds the index of the first or the last index of a given substring.
 * @param string String to be searched for
 * @param substring Substring to be searched for.
 * @param flags SU_NONE | SU_IGNORE_CASE | SU_LAST
 * @return Index of the starting position of the found substring.
 */
int SU_String_index_of(const String str, const String substr, size_t len, bool ignore_case);

/**
 * Counts occurrences that the delimiter (or substring) occurs in a string.
 * @param string String to search
 * @param delimiter Delimiter or Substring to search for
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return Amount of times the delimiter appears in your string, or 0 if NULL string passed
 */
unsigned int SU_String_count(const String str, const String substr, size_t len, bool ignore_case);

/**
 * Returns a substring from between a start and end substring or delimiter in a string.
 * @param string String to be searched.
 * @param start The first substring or delimiter to search for
 * @param end The last substring or delimiter to search for.
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return The substring of what is between start and end, or NULL if NULL string is passed.
 */
String SU_String_between(const String str, const String start, const String end, size_t len, bool ignore_case);

/**
 * Callback function for GCC attribute cleanup. Called when the string leaves the scope of the function.
 * @param string String to be freed.
 */
void SU_String_destroy(String *string_ptr);

#endif	/* SU_String_H */

