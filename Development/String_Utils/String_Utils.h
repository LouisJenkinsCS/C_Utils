/* 
 * File:   String_Utils.h
 * Author: theif519
 *
 * Created on April 8, 2015, 11:24 AM
 */

#ifndef LSTRING_H
#define	LSTRING_H

/**
 * Explanation for definitions below:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * NONE: The default for a function. This is what you pass if you really don't need
 * the other options for any given function.
 * 
 * IGNORE_CASE: For functions that deal with string comparisons and manipulations. This will
 * do just as the macro says, before comparison it will compare the lowercase of whatever
 * it is comparing (without making any modifications) so as the original case doesn't matter.
 * Note: The default is going to be case sensitive.
 * 
 * MODIFY: For functions which deal with string manipulations, this sets the function
 * to modify the string after it's operations are finished, although it will still return
 * it. Note: The default is not going to modify the string.
 * 
 * LAST: For string searches, will attempt to find the last occurrence of whatever it is
 * it will be searching for. Note: The default is going to return the first, there is no middle.
 * (Author Note: Only one function uses this, although I'll be updating and adding more functionality,
 * so there'll be more options for this in the future!)
 * 
 * REVERSE: For string manipulations, this will reverse the string after the function finishes it's operations.
 * Note: The default is going to be normal, non-reversed (duh!)
 * 
 * LOWERCASE: For string manipulations, will convert the string to lowercase.
 * Note: The default is going to be normal.
 * 
 * UPPERCASE: For string manipulations, will convert the string to uppercase.
 * Note: The default is going to be normal.
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * General Notes: More than one parameter can be passed to any given function, 
 * but of course, they must make sense. If you pass a function which supports it both
 * LOWERCASE and UPPERCASE it'll just be the same string you ended up with before
 * passing the parameters. Also, passing nonsensical parameters aren't likely to do anything
 * like NONE | REVERSE will just REVERSE as NONE does nothing. The parameters are
 * very basic and bare bones, but they will be (most likely) developed into something
 * more full fledged.
 */

#define NONE 1 << 0
#define IGNORE_CASE 1 << 1
#define MODIFY  1 << 2
#define LAST 1 << 3
#define REVERSE 1 << 4
#define LOWERCASE 1 << 5
#define UPPERCASE 1 << 6

/*
 * DEBUG: Toggle for whether debug information prints or not
 * 
 * DEBUG_PRINT: Prints a message if Debug is enabled
 * 
 * DEBUG_PRINTF: Prints a formatted message if Debug is enabled (Like printf)
 * 
 * SELECTED: Used to determine whether or not a certain parameter was passed to it,
 * works with more than one. (I.E IGNORE_CASE | IGNORE_CASE | MODIFY)
 * 
 * VALIDATE_PTR: Used to check whether or not a PTR is NULL or not; will print
 * a debug message if debug is enabled informing that the name of the variable is NULL.
 * Also, will return the value passed to it.
 * 
 * VALIDATE_PTR_VOID: Like VALIDATE_PTR, checks for NULL pointer, will print if debug is enabled,
 * except this will return without returning a value.
 * 
 * INIT: Used to shorten the arduous task of initializing all of the callbacks in the constructor.
 * All it does is append the argument passed to it on top of "String_Utils_" to make
 * it a valid function.
 */
#define DEBUG 1
#define DEBUG_PRINT(MESSAGE)(DEBUG ? fprintf(stderr, MESSAGE) : DEBUG)
#define DEBUG_PRINTF(MESSAGE, ...)(DEBUG ? fprintf(stderr, MESSAGE, __VA_ARGS__) : DEBUG)
#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO)) // Bit masking for argument passing.
#define VALIDATE_PTR(PTR, RETURN_VAL) do { if(PTR == NULL){ \
        DEBUG_PRINTF("Error: %s == NULL\n", #PTR); return RETURN_VAL;}} while(0)
#define VALIDATE_PTR_VOID(PTR)do { if(PTR == NULL){ \
        DEBUG_PRINTF("Error: %s == NULL\n", #PTR); return;}} while(0)
#define INIT(CALLBACK)(String_Utils_ ##CALLBACK)
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

/**
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

typedef struct String_Utils String_Utils;

/*
 * String_Utils contains a collection of callbacks for every function with minimal documentation
 * to describe what each does, enough to give a gist of what it does if it's not entirely clear.
 */
struct String_Utils {
    int (*compare)(char *string_one, char *string_two, int parameters); // Compares two strings. (string_one - string_two) (Options: IGNORE_CASE)
    int (*contains)(char *string, char *search, int parameters); // Checks if a string is inside of another string. 1 if true, 0 if false (Options: IGNORE_CASE)
    int (*equals)(char *string_one, char *string_two, int parameters); // Checks if two strings are equal. 1 if equal, 0 if not. (Options: IGNORE_CASE)
    int (*length)(char *string); // Returns length of string.
    int (*count)(char *string, char *substring, int parameter); // Returns occurrences of substring in string. (Options: IGNORE_CASE)
    int (*starts_with)(char *string, char *substring, int parameter); // Checks if string starts with substring (Options: IGNORE_CASE)
    int (*ends_with)(char *string, char *token, int parameter); // Checks if string ends with substring (Options: IGNORE_CASE)
    int (*index_of)(char *string, char *token, int parameter); // Returns start index of substring in string. (Options: IGNORE_CASE)
    char (*char_at)(char *string, unsigned int index); // Returns the character at index in string (or last char if out of bounds)
    char *(*substring)(char *string, unsigned int begin, unsigned int end, int parameter); // Returns a substring from beginning to end of a string. (Options: See Copy)
    char *(*between)(char *string, char *start, char *end, int parameter); // Returns a substring between the first occurrences of the two substrings given. (Options: MODIFY, IGNORE_CASE)
    char *(*copy)(char *string, int parameter); // Returns a copy of the string (Options: LOWERCASE, UPPERCASE, REVERSE)
    char *(*concat)(char *string_one, char *string_two, int parameters); // Concatenates two strings. (Options: MODIFY)
    char *(*capitalize)(char *string, int parameter); // Capitalizes the first character in the first word. (Options: Modify)
    char *(*join)(char **array_of_strings, char *delimiter,  size_t *size, int parameter); // Joins an array of strings into one string. (Options: NONE)
    char *(*trim)(char *string, int parameter); // Trims white spaces at beginning and end of string. (Options: MODIFY)
    char *(*set)(char **string_one, char *string_two, int parameter); // Sets first string to second string. (Options: See Copy)
    char *(*reverse)(char *string, int parameter); // Reverses the string. (Options: MODIFY);
    char *(*concat_all)(int parameter, unsigned int amount, char *string, ...); // Concatenates all strings passed to it into one. (Options: MODIFY)
    char *(*replace)(char *string, char old_char, char new_char, int parameter); // Replaces all of a char with another. (Options: MODIFY, IGNORE_CASE)
    char *(*from)(char *string, unsigned int index, int parameter); // Returns a substring from index to end of string. (Options: MODIFY)
    char *(*from_token)(char *string, char *substring, int parameter); // Returns a substring from a substring (Option: MODIFY, FIRST, LAST)
    char *(*to_lowercase)(char *string, int parameter); // Returns an lowercase string. (Options: MODIFY)
    char *(*to_uppercase)(char *string, int parameter); // Returns an uppercase string. (Options: MODIFY)
    char **(*split)(char *string, char *delimiter, size_t *size, int parameter); // Splits a string into an array of strings based on a delimiter. (Options: NONE)
    char **(*combine)(char **array_one, size_t *array_one_size, char **array_two, size_t *array_two_size, int parameter); // Not implemented.
    char **(*add_to)(char **array, char *string, size_t *size, int parameter); // Not implemented
    char **(*remove_from)(char **array, char *string, size_t *size, int parameter); // Not implemented
    char **(*sort)(char **array, size_t *size, int parameter); // Not implemented
    void (*free_array)(char **array, size_t size); // Frees an array of strings. NOTE: This function frees the contents, but cannot NULL the pointer
    unsigned int *(*get_bytes)(char *string); // Get an array of bytes of a string.
    void (*update)(String_Utils *self); // Do not use!
};

/**
 * Compares two functions. If string_one > string_two, then returns > 0; 
 * If string_one < string_two, returns < 0;
 * If string_one == string_two, returns 0;
 * @param string_one First string to compare with
 * @param string_two Second string to compare with
 * @param parameters NONE - nothing
 * @return (string_one - string_two) (Read above)
 */
int String_Utils_compare(char *string_one, char *string_two, int parameters);


int String_Utils_contains(char *string, char *search, int parameters);


String_Utils *String_Utils_create(void);


char *String_Utils_to_lowercase(char *string, int parameter);

char *String_Utils_to_uppercase(char *string, int parameter);


char String_Utils_char_at(char *string, unsigned int index);


char *String_Utils_concat(char *string_one, char *string_two, int parameters);

void String_Utils_update(String_Utils *self);


unsigned int *String_Utils_get_bytes(char *string);


int String_Utils_equals(char *string_one, char *string_two, int parameter);

char *String_Utils_copy(char *string, int parameter);

char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter);

char *String_Utils_from(char *string, unsigned int index, int parameter);

char *String_Utils_from_token(char *string, char *delimiter, int parameter);

char *String_Utils_concat_all(int parameter, unsigned int amount, char *string, ...);

char *String_Utils_set(char **string_one, char *string_two, int parameter);

char *String_Utils_reverse(char *string, int parameter);

char *String_Utils_join(char **array_of_strings, char *delimiter, size_t *size, int parameter);

char *String_Utils_replace(char *string, char old_char, char new_char, int parameter);

int String_Utils_starts_with(char *string, char *find, int parameter);

int String_Utils_ends_with(char *string, char *find, int parameter);

int String_Utils_length(char *string);

void String_Utils_free_array(char **array, size_t size);

char *String_Utils_substring(char *string, unsigned int begin, unsigned int end, int parameter);

char *String_Utils_capitalize(char *string, int parameter);

char *String_Utils_trim(char *string, int parameter);

int String_Utils_index_of(char *string, char *token, int parameter);

int String_Utils_count(char *string, char *delimiter, int parameter);

char *String_Utils_between(char *string, char *start, char *end, int parameter);
#endif	/* LSTRING_H */

