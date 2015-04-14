/*
 * @Author: Louis Jenkins
 * @Version: 1.0
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
 * Another thing String_Utils offers is a super-cool (IMO, as the creator) idea
 * of using a mega-struct which serves as a callback-machine, WITH basic documentation.
 * If contains a callback function to every single function created here, and makes it a lot
 * easier to call my functions too. For example, lets say you want to concat two strings.
 * Normally you'd have to call String_Utils_concat(...), which can be rather long if you're
 * calling it String_Utils_* over and over, even nested in a statement, so a solution
 * I devised was, lets say you have an instnace of the struct called su, then
 * it's a lot easier to call su->concat than it is to call String_Utils_concat. 
 * The next cool part is the documentation! At least in NetBeans, when you 
 * dereference the struct (or just access the member variable if you prefer that)
 * you can easily see an alphabetically sorted list of all of the functions as
 * well a short 1 - 2 sentence description of each function.
 * 
 * I hope you enjoy my first project as much as I will, I worked very hard on it.
 * Hope it shows! Enjoy!
 */

#ifndef LSTRING_H
#define	LSTRING_H

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
 * REVERSE: For string manipulations, this will reverse the string after the function finishes it's operations.
 * Note: The default is going to be normal, non-reversed (duh!)
 */
#define REVERSE 1 << 4

/**
 * LOWERCASE: For string manipulations, will convert the string to lowercase.
 * Note: The default is going to be normal.
 */
#define LOWERCASE 1 << 5

/**
 * UPPERCASE: For string manipulations, will convert the string to uppercase.
 * Note: The default is going to be normal.
 */
#define UPPERCASE 1 << 6

/**
 * DEBUG: Toggle for whether debug information prints or not
 */
#define DEBUG 1

/**
 * DEBUG_PRINT: Prints a message if Debug is enabled
 */
#define DEBUG_PRINT(MESSAGE)(DEBUG ? fprintf(stderr, MESSAGE) : DEBUG)

/**
 * DEBUG_PRINTF: Prints a formatted message if Debug is enabled (Like printf)
 */
#define DEBUG_PRINTF(MESSAGE, ...)(DEBUG ? fprintf(stderr, MESSAGE, __VA_ARGS__) : DEBUG)

/**
 * SELECTED: Uses bit masking to determine whether or not a certain parameter was passed to it,
 * works with more than one. (I.E IGNORE_CASE | IGNORE_CASE | MODIFY)
 */
#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO))

/**
 * VALIDATE_PTR: Used to check whether or not a PTR is NULL or not; will print
 * a debug message if debug is enabled informing that the name of the variable is NULL.
 * Also, will return the value passed to it.
 */
#define VALIDATE_PTR(PTR, RETURN_VAL) do { if(PTR == NULL){ \
        DEBUG_PRINTF("Error: %s == NULL\n", #PTR); return RETURN_VAL;}} while(0)

/**
 * VALIDATE_PTR_VOID: Like VALIDATE_PTR, checks for NULL pointer, will print if debug is enabled,
 * except this will return without returning a value.
 */
#define VALIDATE_PTR_VOID(PTR)do { if(PTR == NULL){ \
        DEBUG_PRINTF("Error: %s == NULL\n", #PTR); return;}} while(0)

/**
 * INIT: Used to shorten the arduous task of initializing all of the callbacks in the constructor.
 * All it does is append the argument passed to it on top of "String_Utils_" to make
 * it a valid function.
 */
#define INIT(CALLBACK)(String_Utils_ ##CALLBACK)


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

/**
 * String_Utils contains a collection of callbacks for every function with minimal documentation
 * to describe what each does, enough to give a gist of what it does if it's not entirely clear.
 */
typedef struct String_Utils String_Utils;


struct String_Utils {
    /// Compares two strings. (string_one - string_two) (Options: IGNORE_CASE)
    int (*compare)(char *string_one, char *string_two, int parameters); 
    /// Checks if a string is inside of another string. 1 if true, 0 if false (Options: IGNORE_CASE)
    int (*contains)(char *string, char *search, int parameters); 
    /// Checks if two strings are equal. 1 if equal, 0 if not. (Options: IGNORE_CASE)
    int (*equals)(char *string_one, char *string_two, int parameters); 
    /// Returns length of string.
    int (*length)(char *string); 
    /// Returns occurrences of substring in string. (Options: IGNORE_CASE)
    int (*count)(char *string, char *substring, int parameter); 
    /// Checks if string starts with substring (Options: IGNORE_CASE)
    int (*starts_with)(char *string, char *substring, int parameter); 
    /// Checks if string ends with substring (Options: IGNORE_CASE)
    int (*ends_with)(char *string, char *token, int parameter); 
    /// Returns start index of substring in string. (Options: IGNORE_CASE)
    int (*index_of)(char *string, char *substring, int parameter); 
    /// Returns the character at index in string (or last char if out of bounds)
    char (*char_at)(char *string, unsigned int index); 
    /// Returns a substring from beginning to end of a string. (Options: See Copy)
    char *(*substring)(char *string, unsigned int begin, unsigned int end, int parameter); 
    /// Returns a substring between the first occurrences of the two substrings given. (Options: MODIFY, IGNORE_CASE)
    char *(*between)(char *string, char *start, char *end, int parameter); 
    /// Returns a copy of the string (Options: LOWERCASE, UPPERCASE, REVERSE)
    char *(*copy)(char *string, int parameter); 
    /// Concatenates two strings. (Options: MODIFY)
    char *(*concat)(char *string_one, char *string_two, int parameters); 
    /// Capitalizes the first character in the first word. (Options: Modify)
    char *(*capitalize)(char *string, int parameter); 
    /// Joins an array of strings into one string. (Options: NONE)
    char *(*join)(char **array_of_strings, char *delimiter,  size_t *size, int parameter); 
    /// Trims white spaces at beginning and end of string. (Options: MODIFY)
    char *(*trim)(char *string, int parameter); 
    /// Sets first string to second string. (Options: See Copy)
    char *(*set)(char **string_one, char *string_two, int parameter); 
    /// Reverses the string. (Options: MODIFY);
    char *(*reverse)(char *string, int parameter); 
    /// Concatenates all strings passed to it into one. (Options: MODIFY)
    char *(*concat_all)(int parameter, unsigned int amount, char *string, ...); 
    /// Replaces all of a char with another. (Options: MODIFY, IGNORE_CASE)
    char *(*replace)(char *string, char old_char, char new_char, int parameter); 
    /// Returns a substring from index to end of string. (Options: MODIFY)
    char *(*from)(char *string, unsigned int index, int parameter); 
    /// Returns a substring from a substring (Option: MODIFY, FIRST, LAST)
    char *(*from_token)(char *string, char *substring, int parameter); 
    /// Returns an lowercase string. (Options: MODIFY)
    char *(*to_lowercase)(char *string, int parameter); 
    /// Returns an uppercase string. (Options: MODIFY)
    char *(*to_uppercase)(char *string, int parameter); 
    /// Splits a string into an array of strings based on a delimiter. (Options: NONE)
    char **(*split)(char *string, char *delimiter, size_t *size, int parameter); 
    /// Not implemented.
    char **(*combine)(char **array_one, size_t *array_one_size, char **array_two, size_t *array_two_size, int parameter); 
    /// Not implemented
    char **(*add_to)(char **array, char *string, size_t *size, int parameter); 
    /// Not implemented
    char **(*remove_from)(char **array, char *string, size_t *size, int parameter); 
    /// Not implemented
    char **(*sort)(char **array, size_t *size, int parameter); 
    /// Frees an array of strings. NOTE: This function frees the contents, but cannot NULL the pointer
    void (*free_array)(char **array, size_t size); 
    /// Get an array of bytes of a string.
    unsigned int *(*get_bytes)(char *string); 
    /// Do not use!
    void (*update)(String_Utils *self); 
};

/**
 * Compares two functions. If string_one > string_two, then returns > 0; 
 * If string_one < string_two, returns < 0;
 * If string_one == string_two, returns 0;
 * @param string_one First string to compare with
 * @param string_two Second string to compare with
 * @param parameters NONE
 * @return (string_one - string_two) (Read above)
 */
int String_Utils_compare(char *string_one, char *string_two, int parameters);

/**
 * Checks to see if the string contains the substring. 
 * @param string The string
 * @param search Substring to find
 * @param parameters NONE | IGNORE_CASE
 * @return 1 if found, 0 if not found or 0 if a NULL string is passed.
 */
int String_Utils_contains(char *string, char *search, int parameters);

/**
 * Creates the mega-struct Struct_Utils and initializes all implemented callbacks.
 * @return Pointer to the initialized struct or NULL if could not initialize.
 */
String_Utils *String_Utils_create(void);

/**
 * Creates a copy of the string and converts it to lowercase. Either returns the copy
 * or modifies the original string depending on the parameter passed.
 * @param string String to be manipulated
 * @param parameter NONE | MODIFY
 * @return Lowercase string, or NULL if passed a NULL string.
 */
char *String_Utils_to_lowercase(char *string, int parameter);

/**
 * Creates a copy of the string and converts it to uppercase. Either returns the copy
 * or modifies the original string depending on the parameter passed.
 * @param string String to be modified
 * @param parameter NONE | MODIFY
 * @return Uppercase string, or NULL if passed a NULL string.
 */
char *String_Utils_to_uppercase(char *string, int parameter);

/**
 * Returns the char at the position, but if the index is greater than the overall
 * length of the string, the index is set to be the very last character in the string.
 * @param string String to retrieve character from.
 * @param index Index of the char.
 * @return The char at the requested index or the last index if index > strlen(string) - 1 or NULL if NULL string passed.
 */
char String_Utils_char_at(char *string, unsigned int index);

/**
 * Concatenates two strings together. Depending on parameters, it will modify the
 * the first string as it's destination, or it will return a separate string.
 * @param string_one The string the second string is going to be concatenated onto.
 * @param string_two The string which will be concatenated on the first string.
 * @param parameters NONE | MODIFY
 * @return Returns the concatenated string or NULL if NULL string passed.
 */
char *String_Utils_concat(char *string_one, char *string_two, int parameters);

/**
 * Unused, will remove at later date if no use has been found.
 * @param self The String_Utils struct.
 */
void String_Utils_update(String_Utils *self);

/**
 * Returns an array of bytes of the string passed. 
 * Note: A char is a byte, but this will return the number representation of
 * said bytes, an array of them for each character in the string.
 * Author's Note: A bytes_to_string method may be added at a later
 * date if I ever find a use for this. Was added because Java has one like this.
 * @param string The string to turn into an array of bytes.
 * @return Array of bytes (unsigned int *) or NULL if NULL string passed.
 */
unsigned int *String_Utils_get_bytes(char *string);

/**
 * Checks to see if two strings are equal.
 * @param string_one First string to compare
 * @param string_two Second string to compare
 * @param parameter NONE | IGNORE_CASE
 * @return 1 if equal, 0 if not or NULL string passed. 
 */
int String_Utils_equals(char *string_one, char *string_two, int parameter);

/**
 * Creates a copy of the given string. Depending on parameters, it can return
 * a variant of the string, I.E REVERSE returns a copy of the string in REVERSE.
 * @param string String to return a copy of.
 * @param parameter NONE | REVERSE | LOWERCASE | UPPERCASE
 * @return A copy of the string, differs based on parameter passed, or NULL if NULL string passed.
 */
char *String_Utils_copy(char *string, int parameter);

/**
 * Splits a string into an array of strings based on delimiter passed. It should
 * be noted that size must be initialized, because the size of the new string array
 * is recorded in the passed size_t pointer.
 * @param string String to be split.
 * @param delimiter Delimiter to look for when splitting.
 * @param size Records size of the string array
 * @param parameter NONE
 * @return The array of strings, plus sets size to record the size of the array; or NULL if NULL string passed (or size if NULL).
 */
char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter);

/**
 * Returns a copy of the string from the given index, or the last index of the string
 * if index > strlen(string) - 1.
 * @param string String to be operated on.
 * @param index The start of where in the string you want return.
 * @param parameter NONE | MODIFY
 * @return A copy of the string starting at the position, or NULL if NULL string passed.
 */
char *String_Utils_from(char *string, unsigned int index, int parameter);

/**
 * Returns a copy of the string from the first (or last) substring is found.
 * @param string String to search
 * @param substring Substring to search for
 * @param parameter NONE | MODIFY | LAST
 * @return A copy of the string from where the substring is found, NULL if not found or if NULL string passed.
 */
char *String_Utils_from_token(char *string, char *substring, int parameter);

/**
 * Concatenates all strings passed to it.
 * @param parameter NONE | MODIFY
 * @param amount Amount of strings to be concatenated
 * @param string The first string to be passed to it
 * @param ... The rest of the strings to be passed.
 * @return The concatenated string, or NULL if NULL string passed.
 */
char *String_Utils_concat_all(int parameter, unsigned int amount, char *string, ...);

/**
 * Sets one equal to another string.
 * @param string_one The reference to the string to be changed (Use & operator)
 * @param string_two The second string it is going be set to.
 * @param parameter NONE | REVERSE | LOWERCASE | UPPERCASE
 * @return string_one which it sets, or NULL if NULL string passed.
 */
char *String_Utils_set(char **string_one, char *string_two, int parameter);

/**
 * Reverses the given string.
 * @param string String to be operated on.
 * @param parameter NONE | MODIFY
 * @return The reversed string or NULL if NULL string passed.
 */
char *String_Utils_reverse(char *string, int parameter);

/**
 * Joins an array of strings together into one big string with the delimiter prepended to each string.
 * @param array_of_strings The array of strings to be joined.
 * @param delimiter Delimiter to be prepended to each string after the first
 * @param size The size of the array
 * @param parameter NONE
 * @return The new string that was joined, or NULL if NULL string or array of strings passed.
 */
char *String_Utils_join(char **array_of_strings, char *delimiter, size_t *size, int parameter);

/**
 * Replaces all of one character in a string with another character.
 * @param string The string the characters are to be replaced
 * @param old_char The characters to be found
 * @param new_char The characters that will replace the old_char
 * @param parameter NONE | IGNORE_CASE | MODIFY
 * @return The new string with replaced characters.
 */
char *String_Utils_replace(char *string, char old_char, char new_char, int parameter);

/**
 * Checks to see if a string starts with a substring
 * @param string The string to check
 * @param find The substring to check for.
 * @param parameter NONE | IGNORE_CASE
 * @return 1 if true, 0 if false or if NULL string passed.
 */
int String_Utils_starts_with(char *string, char *find, int parameter);

/**
 * Checks to see if a string ends with a substring.
 * @param string The string to check.
 * @param find The substring to check for.
 * @param parameter NONE | IGNORE_CASE
 * @return  1 if true, 0 if false or if NULL string passed
 */
int String_Utils_ends_with(char *string, char *find, int parameter);

/**
 * Returns the length of the string passed.
 * @param string String to get the length of
 * @return strlen(string) or 0 if NULL_STRING
 */
int String_Utils_length(char *string);

/**
 * Experimental Function: Frees all contents of the array
 * NOTE: Does not NULL the pointer to the array, hence accessing the array after
 * leads to undefined behavior.
 * @param array The array of free memory of
 * @param size Size of the array.
 */
void String_Utils_free_array(char **array, size_t size);

/**
 * Returns a substring of the string.
 * @param string String to get a substring of.
 * @param begin The beginning index.
 * @param end The end index.
 * @param parameter NONE | MODIFY | REVERSE | LOWERCASE | UPPERCASE
 * @return The substring of the string, or NULL if NULL string passed.
 */
char *String_Utils_substring(char *string, unsigned int begin, unsigned int end, int parameter);

/**
 * Capitalizes the first character in the string.
 * @param string The string to capitalize.
 * @param parameter NONE | MODIFY
 * @return The capitalized string or NULL if NULL string passed..
 */
char *String_Utils_capitalize(char *string, int parameter);

/**
 * Trims the string of all leading and trailing spaces.
 * @param string String to be trimmed.
 * @param parameter NONE | MODIFY
 * @return Trimmed string or NULL if NULL string passed..
 */
char *String_Utils_trim(char *string, int parameter);

/**
 * Finds the index of the first or the last index of a given substring.
 * @param string String to be searched for
 * @param substring Substring to be searched for.
 * @param parameter NONE | IGNORE_CASE | LAST
 * @return Index of the starting position of the found substring, or -1 if not found or NULL string passed.
 */
int String_Utils_index_of(char *string, char *substring, int parameter);

/**
 * Counts occurrences that the delimiter (or substring) occurs in a string.
 * @param string String to search
 * @param delimiter Delimiter or Substring to search for
 * @param parameter NONE | IGNORE_CASE
 * @return Amount of times the delimiter appears in your string, or 0 if NULL string passed
 */
int String_Utils_count(char *string, char *delimiter, int parameter);

/**
 * Returns a substring from between a start and end substring or delimiter in a string.
 * @param string String to be searched.
 * @param start The first substring or delimiter to search for
 * @param end The last substring or delimiter to search for.
 * @param parameter NONE | IGNORE_CASE
 * @return The substring of what is between start and end, or NULL if NULL string is passed.
 */
char *String_Utils_between(char *string, char *start, char *end, int parameter);
#endif	/* LSTRING_H */

