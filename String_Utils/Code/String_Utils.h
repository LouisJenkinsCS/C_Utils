/*
 * @Author: Louis Jenkins
 * @Version: 1.2
 * 
 * String_Utils is a string library based off of Java's own String library, except
 * the library works on char * over a string object (or struct). It is a lightweight,
 * relatively speedy library which relies upon the libc and glibc. 
 * 
 * String_Utils also features passing flags with the '|' operator, for example,
 * SU_IGNORE_CASE|SU_LAST does exactly as it says, ignore case for string comparison,
 * and return the last string that matches the criteria. For default behavior, you
 * pass SU_NONE flag instead. 
 * 
 * Some functions require passing the address of the string (&) for possible modification
 * of the original string, as String_Utils never modifies the string passed, by default,
 * so it is safe to use string literals unless passing SU_MODIFY flag.
 */

#ifndef STRING_UTILS_H
#define	STRING_UTILS_H

/**
 * Default behavior for a function. 
 */
#define SU_NONE 1 << 0

/**
 * Ignores case for string comparison.
 */
#define SU_IGNORE_CASE 1 << 1

/**
 * Modifies the original string passed to it, A.K.A the one you pass the address of.
 * Note that SU_MODIFY frees the original string, so string literals will cause a segmentation
 * fault.
 */
#define SU_MODIFY  1 << 2

/**
 * Returns the last string or substring for the function.
 */
#define SU_LAST 1 << 3
/**
 * Uses GCC attributes to cleanup strings after leaving the scope of the function.
 * Used as a "feature", it's available without String_Utils, and requires GCC, but
 * it is a nice enough feature. To use it, declare a string like so:
 * 
 * char *string TEMP = "Example String";
 * 
 * When the scope of the variable is left, it will call the cleanup function,
 * String_Utils_Destroy, and handle everything for you.
 */
#define TEMP __attribute__ ((__cleanup__(String_Utils_destroy)))

#include <stdlib.h> /* Can't malloc without it */
#include <string.h> /* Standard libc library. */
#include <stdio.h> /* ??? */
#include <stdarg.h> /* For variadic function concat_all */
#include <assert.h> /* Asserts NULL parameters */
#include <ctype.h> /* To trim strings and check isspace(...). */


/**
 * Compares two strings.
 * @param string_one First string.
 * @param string_two Second string.
 * @param flagss SU_NONE | SU_IGNORE_CASE
 * @return (string_one - string_two).
 */
int String_Utils_compare(const char *string_one, const char *string_two, int flagss);

/**
 * Checks to see if the string contains the substring. 
 * @param string The string
 * @param search Substring to find
 * @param flagss SU_NONE | SU_IGNORE_CASE
 * @return 1 if found, 0 if not found.
 */
int String_Utils_contains(const char *string, const char *search, int flagss);

/**
 * Creates a copy of the string and converts it to lowercase.
 * @param string Pointer to the String to be manipulated
 * @param flags SU_NONE | SU_MODIFY
 * @return Lowercase string.
 */
char *String_Utils_to_lowercase(char **string, int flags);

/**
 * Creates a copy of the string and converts it to uppercase. Either returns the copy
 * or modifies the original string depending on the flags passed.
 * @param string Pointer to the String to be modified
 * @param flags SU_NONE | SU_MODIFY
 * @return Uppercase string.
 */
char *String_Utils_to_uppercase(char **string, int flags);

/**
 * Returns the char at the position. If the index is greater than the overall
 * length of the string, the last character in the string is returned instead.
 * @param string String to retrieve character from.
 * @param index Index of the char.
 * @return The character at the index of the string, or the last character if overflow.
 */
char String_Utils_char_at(const char *string, unsigned int index);

/**
 * Concatenates two strings together.
 * @param string_one Pointer to the The first string.
 * @param string_two The second string.
 * @param flagss SU_NONE | SU_MODIFY
 * @return Returns the concatenated string.
 */
char *String_Utils_concat(char **string_one, const char *string_two, int flagss);

/**
 * Checks to see if two strings are equal.
 * @param string_one First string to compare
 * @param string_two Second string to compare
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return 1 if equal, 0 if not.
 */
int String_Utils_equals(const char *string_one, const char *string_two, int flags);

/**
 * Splits a string into an array of strings based on delimiter passed. It should
 * be noted that size must be initialized, because the size of the new string array
 * is recorded in the passed size_t pointer.
 * @param string String to be split.
 * @param delimiter Delimiter to look for when splitting.
 * @param size Records size of the string array.
 * @return The array of strings, plus sets size to record the size of the array.
 */
char **String_Utils_split(const char *string, const char *delimiter, size_t *size);

/**
 * Returns a copy of the string from the given index. Index must be less than
 * the size of the string, else will return NULL to prevent overflow.
 * @param string Pointer to the String to be operated on.
 * @param index The start of where in the string you want return.
 * @param flags SU_NONE | SU_MODIFY
 * @return A copy of the string starting at the position, or NULL if potential overflow.
 */
char *String_Utils_from(char **string, unsigned int index, int flags);

/**
 * Returns a copy of the string from the first (or last) substring is found.
 * @param string Pointer to the String to search
 * @param substring Substring to search for
 * @param flags SU_NONE | SU_IGNORE_CASE |  SU_MODIFY | SU_LAST
 * @return A copy of the string from where the substring is found, NULL if not found.
 */
char *String_Utils_from_token(char **string, const char *substring, int flags);

/**
 * Concatenates all strings passed to it.
 * @param flags NONE | MODIFY
 * @param amount Amount of strings to be concatenated
 * @param string Pointer to the The first string to be passed to it
 * @param ... The rest of the strings to be passed.
 * @return The concatenated string.
 */
char *String_Utils_concat_all(int flags, size_t amount, char **string, ...);

/**
 * Sets one equal to another string.
 * @param string_one The reference to the string to be changed (Use & operator)
 * @param string_two The second string it is going be set to.
 * @return string_one which it sets.
 */
char *String_Utils_set(char **string_one, const char *string_two);

/**
 * Reverses the given string.
 * @param string Pointer to the String to be operated on.
 * @param flags SU_NONE | SU_MODIFY
 * @return The reversed string.
 */
char *String_Utils_reverse(char **string, int flags);

/**
 * Joins an array of strings together into one big string with the delimiter prepended to each string after the first.
 * @param array_of_strings The array of strings to be joined.
 * @param delimiter Delimiter to be prepended to each string after the first
 * @param size The size of the array
 * @return The new string that was joined.
 */
char *String_Utils_join(const char **array_of_strings, const char *delimiter, size_t size);

/**
 * Replaces all of one character in a string with another character.
 * @param string Pointer to the The string the characters are to be replaced
 * @param old_char The characters to be found
 * @param new_char The characters that will replace the old_char
 * @param flags SU_NONE | SU_IGNORE_CASE | SU_MODIFY
 * @return The new string with replaced characters.
 */
char *String_Utils_replace(char **string, char old_char, char new_char, int flags);

/**
 * Checks to see if a string starts with a substring
 * @param string he string to check.
 * @param find The substring to check for.
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return 1 if true, 0 if false.
 */
int String_Utils_starts_with(const char *string, const char *find, int flags);

/**
 * Checks to see if a string ends with a substring.
 * @param string The string to check.
 * @param find The substring to check for.
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return  1 if true, 0 if false.
 */
int String_Utils_ends_with(const char *string, const char *find, int flags);

/**
 * Returns a substring of the string.
 * @param string Pointer to the String to get a substring of.
 * @param begin The beginning index.
 * @param end The end index.
 * @param flags SU_NONE | SU_MODIFY
 * @return The substring of the string.
 */
char *String_Utils_substring(char **string, unsigned int begin, unsigned int end, int flags);

/**
 * Capitalizes the first character in the string.
 * @param string Pointer to the string to capitalize.
 * @param flags SU_NONE | SU_MODIFY
 * @return The capitalized string.
 */
char *String_Utils_capitalize(char **string, int flags);

/**
 * Trims the string of all leading and trailing spaces.
 * @param string Pointer to the String to be trimmed.
 * @param flags SU_NONE | SU_MODIFY
 * @return Trimmed string.
 */
char *String_Utils_trim(char **string, int flags);

/**
 * Finds the index of the first or the last index of a given substring.
 * @param string String to be searched for
 * @param substring Substring to be searched for.
 * @param flags SU_NONE | SU_IGNORE_CASE | SU_LAST
 * @return Index of the starting position of the found substring.
 */
int String_Utils_index_of(const char *string, const char *substring, int flags);

/**
 * Counts occurrences that the delimiter (or substring) occurs in a string.
 * @param string String to search
 * @param delimiter Delimiter or Substring to search for
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return Amount of times the delimiter appears in your string, or 0 if NULL string passed
 */
int String_Utils_count(const char *string, const char *substring, int flags);

/**
 * Returns a substring from between a start and end substring or delimiter in a string.
 * @param string String to be searched.
 * @param start The first substring or delimiter to search for
 * @param end The last substring or delimiter to search for.
 * @param flags SU_NONE | SU_IGNORE_CASE
 * @return The substring of what is between start and end, or NULL if NULL string is passed.
 */
char *String_Utils_between(const char *string, const char *start, const char *end, int flags);

/**
 * Callback function for GCC attribute cleanup. Called when the string leaves the scope of the function.
 * @param string String to be freed.
 */
void String_Utils_destroy(char **string);

#endif	/* STRING_UTILS_H */

