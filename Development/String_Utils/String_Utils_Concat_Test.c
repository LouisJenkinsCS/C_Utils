#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
 
// Had to adjust macros a bit.
 
char *String_Utils_concat(char *string_one, char *string_two, int parameter);
char *String_Utils_set(char **string_one, char *string_two, int parameter);
char *String_Utils_copy(char *string, int parameter);
void testString_Utils_concat();
 
#define NONE 1 << 0
#define MODIFY 1 << 1
#define DEBUG 1
#define DEBUG_PRINTF(MESSAGE, ...)(DEBUG ? fprintf(stderr, MESSAGE, __VA_ARGS__) : DEBUG)
#define VALIDATE_PTR(PTR, RETURN_VAL) do { if(PTR == NULL){ \
DEBUG_PRINTF("Error: %s == NULL\n", #PTR); return RETURN_VAL;}} while(0)
#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO))
 
 
char *String_Utils_concat(char *string_one, char *string_two, int parameter) { // For this demonstration, MODIFY is passed
char *temp = NULL;
// Validate_PTR checks if pointer is null, and if so returns the second parameter, I.E NULL
VALIDATE_PTR(string_one, NULL);
VALIDATE_PTR(string_two, NULL);
// Allocate temporary string to size of two strings, + 1 for null terminator
temp = malloc(strlen(string_one) + strlen(string_two) + 1);
// Zeroes the struct to clean it.
memset(temp, 0, strlen(string_one) + strlen(string_two) + 1);
// Copies the first string to temp
strcpy(temp, string_one);
// Concatenates string_two to temp
strcat(temp, string_two);
// Based on parameters below, will modify the original screen, or just return the new string.
if (SELECTED(parameter, MODIFY)) {
String_Utils_set(&string_one, temp, NONE); // The call to set. Passes address of the string
// Free temp as no longer needed.
free(temp);
return string_one;
} // Has to return something, may as well be the same string 
else return temp; // Not modifying the original string is the default, even if there was no valid parameter.
}
 
char *String_Utils_set(char **string_one, char *string_two, int parameter) {
VALIDATE_PTR(string_one, NULL);
VALIDATE_PTR(*string_one, NULL); // Also checks if the string is null through the reference.
VALIDATE_PTR(string_two, NULL);
printf("Value of *string_one = %s\nMemory Address of *String_one = %p\n String_two = %s\n", *string_one, *string_one, string_two);
char *temp_string_two = String_Utils_copy(string_two, parameter); // Copy returns a copy of the string modified with the parameter
// Frees the string. This WILL SEGFAULT if a normal string literal was passed.
free(*string_one);
*string_one = malloc(strlen(temp_string_two) + 1); // Allocates string_one.
// Clears out the string
memset(*string_one, 0 , strlen(temp_string_two)+1);
// Copy string_two into string_one
strcpy(*string_one, temp_string_two);
free(temp_string_two);
printf("Final result of *String_one = %s\nFinal Memory Address of *String_one = %p\n", *string_one, *string_one);
return *string_one;
}
char *String_Utils_copy(char *string, int parameter) { // Returns a copy of the string modified if parameters are passed.
VALIDATE_PTR(string, NULL);
// Allocates a temporary string to hold copy of string
char *temp = malloc(strlen(string) + 1);
strcpy(temp, string);
// The below will modify temp the appropriate parameter.
//if(SELECTED(parameter, LOWERCASE)) temp = String_Utils_to_lowercase(temp, MODIFY); // MODIFY parameter always results in calling set.
//if(SELECTED(parameter, UPPERCASE)) temp = String_Utils_to_uppercase(temp, MODIFY); // Comment these out if you want to run without it.
//if(SELECTED(parameter, REVERSE)) temp = String_Utils_reverse(temp, MODIFY);
return temp;
}
 
// The test portion
 
void testString_Utils_concat() {
char* string_one = "Hello ";
char* string_two = "World";
char *string_three = String_Utils_copy("", NONE); // In order to change this value, a copy of the string is returned else SEGFAULT
printf("\n\n\nString_Three's value: %s\nString_Three's memory address: %p\n", string_three, string_three);
char *null_string = NULL;
int parameter_one = NONE;
int parameter_two = MODIFY;
char* result_one = String_Utils_concat(string_one, string_two, parameter_one);
char *result_two = String_Utils_concat(string_one, null_string, parameter_one);
String_Utils_concat(string_three, result_one, parameter_two); // Modifies string_three
printf("String_Three's value: %s\nString_Three's memory address: %p\n", string_three, string_three);

if (strcmp(result_one, "Hello World") != 0) {
 	assert(0);
}
if(result_two != NULL) assert(0);
if(strcmp(result_one, string_three) != 0) assert(0);
printf("Passed test: Concat!\n");
//free(string_one);
//free(string_two);
free(string_three);
free(result_one);
free(result_two);
}
 
int main(void){
testString_Utils_concat();
return 0;
} 