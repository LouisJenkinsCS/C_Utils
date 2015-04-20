/*
 * @Author: Louis Jenkins
 * @Version: 1.1
 * 
 * What is String_Utils? String_Utils is basically an attempt at implementing a
 * very useful, somewhat efficient String library with basic string manipulations
 * comparators, and utilities offered in object oriented languages. In fact, this
 * is based on Java's String object's methods, hence the name of most of the functions.
 * 
 * My reason for creating this is, not just for fun, and boy was it ever, but 
 * also because I haven't found any attempt at creating a string library the way
 * I did. I rather dislike the way C's String library handles thing, it's too
 * minimal with how it abstracts and encapsulates it's functions, and plus it 
 * doesn't even have the basic functions that most people use day-to-day, but they
 * do however give you the tools to do it yourself, so I decided to. 
 * 
 * String_Utils has a plethora of well-tested (as well as you can with one person
 * writing this in one week) functions that you've grown to love in OOP languages
 * like Java, I.E Substring, Index_OF, Split/Join, etc. I attempted to implement
 * them as closely as they would be in Java, although of course since C and Java
 * are vastly different languages, with different paradigms, it's impossible
 * to make it exactly like so. 
 *
 * Another nice feature is the passing of parameters to tailor the operation the way you want
 * it if the library function supports it. For instance, one of the problems I had with the standard
 * C library was that strcat modifies the original string AND returns the pointer to the same
 * string it modified. I figured, why not just have an option to modify the string passed or not? 
 * What if I want to work with a string literal? Then with strcat, it would segfault. With my 
 * concat implementation, you can pass a string literal and not have it attempt to modify it, instead
 * create a copy of the string for you. In some functions, multiple parameter passing can be passed
 * with the | operator, I.E 'MODIFY | IGNORE_CASE'
 *
 * Another note, to keep the feature of being able to modify the string, all functions that can modify,
 * even if you do not modify it directly, must pass the address of the string (& operator). For exmaple,
 * String_Utils_concat(&string_one, string_two, MODIFY). A minor inconvenience.
 * 
 * I hope you enjoy my first project as much as I will, I worked very hard on it.
 * Hope it shows! Enjoy!
 */

#ifndef STRING_UTILS_H
#define	STRING_UTILS_H

/*
 * General Notes: More than one parameter can be passed to any given function, 
 * but of course, they must make sense. If you pass a function which supports it both
 * LOWERCASE and UPPERCASE it'll just be the same string you ended up with before
 * passing the parameters. Also, passing nonsensical parameters aren't likely to do anything
 * like NONE | REVERSE will just REVERSE as NONE does nothing. The parameters are
 * very basic and bare bones, but they will be (most likely) developed into something
 * more full fledged.
*/

/**
* After realizing how stupid it was to use a structure to hold merely function pointers
* to all of the functions below, which not only bloated the code by about 100 - 150 LoC
* this macro will fill the need of anything. SU_Concat would do the same, with infinitely less
* memory consumption than a struct having to be allocated, then have ALL 28 - 30 functions initialized
* to each function, then call it like SU->Concat. I've learned my lesson.
*/
 #define SU String_Utils

/**
 * NONE: The default for a function. This is what you pass if you really don't need
 * the other options for any given function.
 */
#define NONE 1 << 0

/**
 * IGNORE_CASE: For functions that deal with string comparisons and manipulations. This will
 * do just as the macro says, before comparison it will compare the lowercase of whatever
 * it is comparing (without making any modifications) so as the original case doesn't matter.
 * Note: The default is going to be case sensitive.*/
#define IGNORE_CASE 1 << 1

/**
 * MODIFY: For functions which deal with string manipulations, this sets the function
 * to modify the string after it's operations are finished, although it will still return
 * it. Note: The default is not going to modify the string.
 */
#define MODIFY  1 << 2

/**
 * LAST: For string searches, will attempt to find the last occurrence of whatever it is
 * it will be searching for. Note: The default is going to return the first, there is no middle.
 * (Author Note: Only one function uses this, although I'll be updating and adding more functionality,
 * so there'll be more options for this in the future!)
 */
#define LAST 1 << 3

/**
 * String_Utils_DEBUG: Toggle for whether debug information prints or not
 */
#define String_Utils_DEBUG 1

/**
 * String_Utils_DEBUG_PRINT: Prints a message if Debug is enabled
 */
#define String_Utils_DEBUG_PRINT(MESSAGE)(String_Utils_DEBUG ? fprintf(stderr, MESSAGE) : String_Utils_DEBUG)

/**
 * String_Utils_DEBUG_PRINTF: Prints a formatted message if Debug is enabled (Like printf)
 */
#define String_Utils_DEBUG_PRINTF(MESSAGE, ...)(String_Utils_DEBUG ? fprintf(stderr, MESSAGE, __VA_ARGS__) : String_Utils_DEBUG)

/**
 * SELECTED: Uses bit masking to determine whether or not a certain parameter was passed to it,
 * works with more than one. (I.E IGNORE_CASE | IGNORE_CASE | MODIFY)
 */
#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO))

#include <stdlib.h> /* Can't malloc without it */
#include <string.h> /* Overly obvious reasons */
#include <stdio.h> /* Obvious reasons */
#include <stdarg.h> /* For variadic function concat_all */
#include <assert.h> /* Asserts NULL parameters */
#include <ctype.h> /* To trim strings and check isspace(...). */


/**
 * Compares two strings.
 * @param string_one First string.
 * @param string_two Second string.
 * @param parameters NONE | IGNORE_CASE
 * @return (string_one - string_two).
 */
int String_Utils_compare(const char *string_one, const char *string_two, int parameters);

/**
 * Checks to see if the string contains the substring. 
 * @param string The string
 * @param search Substring to find
 * @param parameters NONE | IGNORE_CASE
 * @return 1 if found, 0 if not found.
 */
int String_Utils_contains(const char *string, const char *search, int parameters);

/**
 * Creates a copy of the string and converts it to lowercase.
 * @param string Pointer to the String to be manipulated
 * @param parameter NONE | MODIFY
 * @return Lowercase string.
 */
char *String_Utils_to_lowercase(char **string, int parameter);

/**
 * Creates a copy of the string and converts it to uppercase. Either returns the copy
 * or modifies the original string depending on the parameter passed.
 * @param string Pointer to the String to be modified
 * @param parameter NONE | MODIFY
 * @return Uppercase string.
 */
char *String_Utils_to_uppercase(char **string, int parameter);

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
 * @param parameters NONE | MODIFY
 * @return Returns the concatenated string.
 */
char *String_Utils_concat(char **string_one, const char *string_two, int parameters);

/**
 * Returns the string in byte representation.
 * Author's Note: A bytes_to_string method may be added at a later
 * date if I ever find a use for this. Was added because Java has one like this.
 * @param string The string to turn into an array of bytes.
 * @return Array of bytes.
 */
unsigned int *String_Utils_get_bytes(const char *string);

/**
 * Checks to see if two strings are equal.
 * @param string_one First string to compare
 * @param string_two Second string to compare
 * @param parameter NONE | IGNORE_CASE
 * @return 1 if equal, 0 if not.
 */
int String_Utils_equals(const char *string_one, const char *string_two, int parameter);

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
 * @param parameter NONE | MODIFY
 * @return A copy of the string starting at the position, or NULL if potential overflow.
 */
char *String_Utils_from(char **string, unsigned int index, int parameter);

/**
 * Returns a copy of the string from the first (or last) substring is found.
 * @param string Pointer to the String to search
 * @param substring Substring to search for
 * @param parameter NONE | IGNORE_CASE |  MODIFY | LAST
 * @return A copy of the string from where the substring is found, NULL if not found.
 */
char *String_Utils_from_token(char **string, const char *substring, int parameter);

/**
 * Concatenates all strings passed to it.
 * @param parameter NONE | MODIFY
 * @param amount Amount of strings to be concatenated
 * @param string Pointer to the The first string to be passed to it
 * @param ... The rest of the strings to be passed.
 * @return The concatenated string.
 */
char *String_Utils_concat_all(int parameter, size_t amount, char **string, ...);

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
 * @param parameter NONE | MODIFY
 * @return The reversed string.
 */
char *String_Utils_reverse(char **string, int parameter);

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
 * @param parameter NONE | IGNORE_CASE | MODIFY
 * @return The new string with replaced characters.
 */
char *String_Utils_replace(char **string, char old_char, char new_char, int parameter);

/**
 * Checks to see if a string starts with a substring
 * @param string he string to check.
 * @param find The substring to check for.
 * @param parameter NONE | IGNORE_CASE
 * @return 1 if true, 0 if false.
 */
int String_Utils_starts_with(const char *string, const char *find, int parameter);

/**
 * Checks to see if a string ends with a substring.
 * @param string The string to check.
 * @param find The substring to check for.
 * @param parameter NONE | IGNORE_CASE
 * @return  1 if true, 0 if false.
 */
int String_Utils_ends_with(const char *string, const char *find, int parameter);

/**
 * Returns a substring of the string.
 * @param string Pointer to the String to get a substring of.
 * @param begin The beginning index.
 * @param end The end index.
 * @param parameter NONE | MODIFY
 * @return The substring of the string.
 */
char *String_Utils_substring(char **string, unsigned int begin, unsigned int end, int parameter);

/**
 * Capitalizes the first character in the string.
 * @param string Pointer to the string to capitalize.
 * @param parameter NONE | MODIFY
 * @return The capitalized string.
 */
char *String_Utils_capitalize(char **string, int parameter);

/**
 * Trims the string of all leading and trailing spaces.
 * @param string Pointer to the String to be trimmed.
 * @param parameter NONE | MODIFY
 * @return Trimmed string.
 */
char *String_Utils_trim(char **string, int parameter);

/**
 * Finds the index of the first or the last index of a given substring.
 * @param string String to be searched for
 * @param substring Substring to be searched for.
 * @param parameter NONE | IGNORE_CASE | LAST
 * @return Index of the starting position of the found substring.
 */
int String_Utils_index_of(const char *string, const char *substring, int parameter);

/**
 * Counts occurrences that the delimiter (or substring) occurs in a string.
 * @param string String to search
 * @param delimiter Delimiter or Substring to search for
 * @param parameter NONE | IGNORE_CASE
 * @return Amount of times the delimiter appears in your string, or 0 if NULL string passed
 */
int String_Utils_count(const char *string, const char *substring, int parameter);

/**
 * Returns a substring from between a start and end substring or delimiter in a string.
 * @param string String to be searched.
 * @param start The first substring or delimiter to search for
 * @param end The last substring or delimiter to search for.
 * @param parameter NONE | IGNORE_CASE
 * @return The substring of what is between start and end, or NULL if NULL string is passed.
 */
char *String_Utils_between(const char *string, const char *start, const char *end, int parameter);

/**
 * Initialize the garbage collector for functions.
 */
void String_Utils_Init_GC(void);

/**
 * Destroys the garbage collector.
 */
void String_Utils_Destroy_GC(void);
#endif	/* STRING_UTILS_H */

