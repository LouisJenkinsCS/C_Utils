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
 * String Comparisons:
 * 
 * NORMAL: Default mode. Comparisons are case sensitive.
 * 
 * IGNORE_CASE: Comparisons are not case sensitive.
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * String Modifications:
 * 
 * MODIFY: Ensures that the first string will be modified as per use,
 * I.E String_Utils_concat will concatenate string_two on string_one, changing string_one
 * 
 * NO_MODIFY: Ensures that the first string will NOT be modified by using a temporary string literal and returning it.
 * I.E String_Utils_concat will not modify string_one and will return a concatenated string literal.
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * String Searching: (Currently Unimplemented)
 * 
 * FIRST: While searching for a token in a string, the function will return the very first occurrence
 * of the token it finds. 
 * 
 * LAST: While searching for a token in a string, the function will return the very last occurrence
 * of the token it finds.
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define NONE 1 << 0
#define NORMAL 1 << 1
#define IGNORE_CASE 1 << 2
#define MODIFY  1 << 3
#define NO_MODIFY 1 << 4
#define FIRST 1 << 5
#define LAST 1 << 6
#define REVERSE 1 << 7

#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO)) // Bit masking for argument passing.
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

/**
 * It should be noted that String_Utils's "methods" of course, require you to pass a pointer to itself, as
 * C lacks any reflection and a 'this' operator. Secondly, majority of the String_Utils functions that use
 * comparisons use string literals. This is because, returning a new struct would be rather inefficient
 * and comparing two structs is also do-able, but due to the lack of overloading of functions, it'd be
 * much smarter to use string literals as they're much more widely used. 
 * 
 * On this note, I suppose I should state the actual objective of this 'Object'. Unlike in other languages, 
 * String_Utils is not going to be an actual string (obviously) nor have it's benefits as if it were built in. 
 * What it does boast is the ability of having commonly used functions and operations on strings, even a few
 * that aren't currently available in the normal string.h library, or those that are overly complicated
 * that are simplified. So...
 * 
 * 1) String_Utils is not intended to be used as a normal string in other classes...
 * 1A) String_Utils does however give access to functions to manipulate strings easily.
 * 1B) Easier to use than normal string.h library for beginners (Not like they're ever use it)
 * 2) String_Utils will, at some point, inherit from LJObject, as will other LJObject 'subclasses' 
 */

typedef struct String_Utils String_Utils;

/*
 String_Utils is a structure which holds all of the following:
 * 1) The char literal, A.K.A string.
 * 2) The Size and Length of the string.
 * 3) Callback functions for easy string manipulations without having to type arduously long function names.
 */
struct String_Utils {
    void *LinkedList; // List to hold all stored strings.
    char *(*concat)(char *string_one, char *string_two, int parameters);
    char (*char_at)(char *string, unsigned int index);
    int (*compare)(char *string_one, char *string_two, int parameters); // Compares two strings.
    int (*contains)(char *string, char *search, int parameters);
    int (*equals)(char *string_one, char *string_two, int parameters);
    int (*length)(char *string);
    int (*count)(char *string, char *delimiter, int parameter);
    int (*starts_with)(char *string, char *token); // Not Implemented
    int (*ends_with)(char *string, char *token); // Not Implemented
    int (*index_of)(char *string, char *token, int parameter); // Not implemented
    int (*store)(String_Utils *self, char **string, unsigned int amount); // Stores array of strings into linked list; Not Implemented
    char *(*substring)(char *string, unsigned int begin, unsigned int end); // Not Implemented
    char *(*copy)(char *string);
    char *(*capitalize)(char *string, int parameter); // Not implemented
    char *(*join)(char **array_of_strings, size_t *size, int parameter); // Add to constructor
    char *(*trim)(char *string, int parameter); // Trims white spaces at beginning and end of string
    char *(*set)(char *string_one, char *string_two, int parameter);
    char *(*reverse)(char *string, int parameter);
    char *(*concat_all)(int parameter, unsigned int amount, char *string, ...);
    char *(*replace)(char *string, char old_char, char new_char, int parameter); // Add to constructor
    char *(*from)(char *string, unsigned int index, int parameter); // Add to constructor
    char *(*from_token)(char *string, char *delimiter, int parameter); // Testing...
    char *(*to_lower)(char *string, int parameter); // Add to constructor
    char *(*to_upper)(char *string, int parameter); // Add to constructor
    char **(*split)(char *string, char *delimiter, size_t *size, int parameter); // Add to constructor
    char** (*retrieve)(String_Utils *self, unsigned int start, unsigned int end); // Retrieves an array of strings from linked list
    void * (*iterator)(String_Utils *self); // Returns an iterator for the user to use.
    unsigned int *(*get_bytes)(char *string); 
    void (*update)(String_Utils *self); // Remove or find a use for it!
    // Currently not implemented: LinkedList, retrieve, iterator, store.
};

/**
 * Why reinvent the wheel? Simple function that calls strcmp to compare two strings
 * and returns it's result. Takes two parameters, NORMAL or IGNORE_CASE, will default to NORMAL if no
 * valid parameter has been given.
 * @param self This String_Utils, could also be any string, so long as it's another String_Utils.
 * @param str The opposing char literal to compare to.
 * @param parameters NORMAL (0) = Case Sensitive, IGNORE_CASE (1) = Not Case Sensitive
 * @return Returns what strcmp returns.
 */
int String_Utils_compare(char *string_one, char *string_two, int parameters);

/**
 * Also not re-inventing the wheel, basically returns what strcmp returns. Takes two
 * parameters, NORMAL (Case sensitive) and IGNORE_CASE (Case insensitive) for comparisons.
 * @param self This String_Utils
 * @param str The string to check if the String_Utils contains.
 * @param parameters NORMAL (0) = Case Sensitive, IGNORE_CASE (1) = Not Case Sensitive
 * @return Returns what strstr returns.
 */
int String_Utils_contains(char *string, char *search, int parameters);

/**
 * Basic constructor for String_Utils, allocates memory, assigns the passed str as it's value,
 * and initializes it's attributes and callback functions.
 * @param str The string to be given as the initial value.
 * @return The allocated pointer to the String_Utils.
 */
String_Utils *String_Utils_create(void);

/**
 * Used to set a string to lower case, basically used for other String_Utils functions, would be
 * private if it were possible.
 * @param str string to be passed through
 * @return lowercase string.
 */
char *String_Utils_to_lowercase(char *string, int parameter);

char *String_Utils_to_uppercase(char *string, int parameter);

/**
 * Has basic bounds checking, will return the last char if the index surpasses the
 * the String's length, otherwise will return the char at the index. If the the user
 * has changed the String's value directly, errors may occur unless they call the update callback.
 * @param self The String_Utils to be manipulated
 * @param index The index at which to retrieve the character.
 * @return The character at the given index, or the last index of the val.
 */
char String_Utils_char_at(char *string, unsigned int index);

/**
 * Concatenates the String_Utils's value with the passed through string literal.
 * @param self This String_Utils
 * @param str String to be concatenated to the String_Utils.
 */
char *String_Utils_concat(char *string_one, char *string_two, int parameters);
/**
 * This function should be called whenever you decide to manipulate the String_Utils's val without
 * using one of it's callback functions. It updates the String_Utils's size and length of it's string.
 * @param self This String_Utils
 */
void String_Utils_update(String_Utils *self);

/**
 * Converts the current char literal to an unsigned char literal, then converts it
 * to a unsigned int and returns that, which is the equivalent to a byte;
 * @param self This string
 * @return A byte-representation of the String.
 */
unsigned int *String_Utils_get_bytes(char *string);

/**
 * Function that checks whether or not this string is equal to another string literal.
 * @param self This string
 * @param string string literal to compare to
 * @param parameters NORMAL, IGNORE_CASE and anything else defaults to NORMAL
 * @return 0 if false, 1 if true
 */
int String_Utils_equals(char *string_one, char *string_two, int parameter);

char *String_Utils_copy(char *string);

char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter);

char *String_Utils_from(char *string, unsigned int index, int parameter);

char *String_Utils_from_token(char *string, char *delimiter, int parameter);

char *String_Utils_concat_all(int parameter, unsigned int amount, char *string, ...);

char *String_Utils_set(char *string_one, char *string_two, int parameter);

char *String_Utils_reverse(char *string, int parameter);

char *String_Utils_join(char **array_of_strings, size_t *size, int parameter);

char *String_Utils_replace(char *string, char old_char, char new_char, int parameter);
int String_Utils_length(char *string);
#endif	/* LSTRING_H */

