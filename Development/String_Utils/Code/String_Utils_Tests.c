#include "String_Utils.h"
#include <string.h>
#define TEST_ALL_FUNCTIONS testString_Utils_capitalize();\
	testString_Utils_char_at();\
	testString_Utils_compare();\
	testString_Utils_concat();\
	testString_Utils_copy();\
	testString_Utils_count();\
	testString_Utils_create();\
	testString_Utils_ends_with();\
	testString_Utils_equals();\
	testString_Utils_from();\
	testString_Utils_from_token();\
	testString_Utils_get_bytes();\
	testString_Utils_index_of();\
	testString_Utils_join();\
	testString_Utils_length();\
	testString_Utils_replace();\
	testString_Utils_reverse();\
	testString_Utils_set();\
	testString_Utils_split();\
	testString_Utils_starts_with();\
	testString_Utils_substring();\
	testString_Utils_to_lowercase();\
	testString_Utils_to_uppercase();\
	testString_Utils_trim();
void testString_Utils_capitalize() {
    char* string = "hello World";
    char *null_string = NULL; // Tests null pointer as argument
    int parameter = NONE;
    char* result_one = String_Utils_capitalize(string, parameter);
    char* result_two = String_Utils_capitalize(null_string, parameter);
    if (strcmp(result_one, "Hello World") != 0) {
        assert(0);
    }
    if(result_two != NULL) assert(0);
    printf("Passed test: Capitalize!\n");
    
}

void testString_Utils_char_at() {
    char* string = "Hello World";
    char *null_string = NULL;
    unsigned int index_one = 100;
    unsigned int index_two = 5;
    char result_one = String_Utils_char_at(string, index_one);
    char result_two = String_Utils_char_at(string, index_two);
    char result_three = String_Utils_char_at(null_string, index_one);
    if (string[strlen(string)-1] != result_one) { // Tests with out of bounds index
        assert(0);
    }
    if(string[5] != result_two) assert(0); // Tests in-bounds index
    if(result_three != '\0') assert(0);
    free(string);
    free(null_string);
    printf("Passed test: Char_At!\n"); 
}

void testString_Utils_compare() {
    char* string_one = "Hello World";
    char* string_two = "Hello_World";
    char *string_three = "Hello WorlD";
    char *null_string = NULL;
    int parameters = IGNORE_CASE;
    int result_one = String_Utils_compare(string_one, string_two, parameters);
    int result_two = String_Utils_compare(null_string, string_two, parameters);
    int result_three = String_Utils_compare(string_one, string_one, parameters);
    int result_four = String_Utils_compare(string_one, string_three, parameters);
    if (result_one == 0) {
        assert(0);
    }
    if(result_two != -1) assert(0);
    if(result_three != 0) assert(0);
    if(result_four != 0) assert(0);
    free(string_one);
    free(string_two);
    free(string_three);
    printf("Passed test: Compare!\n"); 
}

void testString_Utils_concat() {
    char* string_one = "Hello ";
    char* string_two = "World";
    char *string_three = String_Utils_copy("", NONE); // In order to change this value, a copy of the string is returned
    printf("\n\n\nString_Three's value: %s\nString_Three's memory address: %p\n", string_three, string_three);
    char *null_string = NULL;
    int parameter_one = NONE;
    int parameter_two = MODIFY;
    char* result_one = String_Utils_concat(string_one, string_two, parameter_one);
    char *result_two = String_Utils_concat(string_one, null_string, parameter_one);
    String_Utils_concat(string_three, result_one, parameter_two); // Modifies string_three
    printf("String_Three's value: %s\nString_Three's memory address: %p\n", string_three, string_three);
    //printf("Result_One's val: %s\n", result_one);
    if (strcmp(result_one, "Hello World") != 0) {
        assert(0);
    }

    if(result_two != NULL) assert(0);
    //if(strcmp(result_one, string_three) != 0) assert(0);
    printf("Passed test: Concat!\n");
    free(string_one);
    free(string_two);
    free(string_three);
    free(result_one);
    free(result_two);
}

void testString_Utils_contains() {
    char* string = "Hello World, the weather is nice today, isn't it?";
    char* search = "the";
    char *null_string = NULL;
    int parameters = IGNORE_CASE;
    int result_one = String_Utils_contains(string, search, parameters);
    int result_two = String_Utils_contains(string, null_string, parameters);
    if (result_one != 1) {
        assert(0);
    }
    if(result_two != 0) assert(0);
    printf("Passed test: Contains!\n");
    free(string);
    free(search);
}

void testString_Utils_copy() {
    char* string = "Hello World";
    char *palindrome_string = "racecar";
    char *special_string = "stressed";
    char *null_string = NULL;
    int parameter = NONE;
    int reverse = REVERSE;
    char* result_one = String_Utils_copy(string, parameter);
    char *result_two = String_Utils_copy(palindrome_string, reverse); // If anything but the same string is returned, its bad
    char *result_three = String_Utils_copy(special_string, reverse); // You'll see
    char *result_four = String_Utils_copy(null_string, parameter);
    if (strcmp(string, result_one) != 0) {
        assert(0);
    }
    if(strcmp(palindrome_string, result_two) != 0) assert(0);
    if(strcmp(result_three, "desserts") != 0) assert(0); // Special string indeed!
    if(result_four != NULL) assert(0);
    printf("Passed test: Copy!\n");
    free(string);
    free(palindrome_string);
    free(special_string);
    free(result_one);
    free(result_two);
    free(result_three);
    free(result_four);
}

void testString_Utils_count() {
    char* string = "What is the meaning of the word 'the', when there is the person in the mirror staring back at the recipient?";
    char* delimiter = "the";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    int result_one = String_Utils_count(string, delimiter, parameter);
    int result_two = String_Utils_count(null_string, delimiter, parameter);
    if (result_one != 7) { // there has the word 'the' in it, no spaces
        assert(0);
    }
    if(result_two != 0) assert(0);
    printf("Passed test: Count!\n");
    free(string);
    free(delimiter);
}

void testString_Utils_create() {
    //String_Utils* result = String_Utils_create();
    //if (result == NULL) {
    //    assert(0);
    //}
    //free(result);
    //printf("Passed test: Create!\n");
}

void testString_Utils_ends_with() {
    char* string = "Catch the end of this string";
    char* find_one = "string";
    char *find_two = "END OF THIS STRING";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    int result_one = String_Utils_ends_with(string, find_one, parameter);
    int result_two = String_Utils_ends_with(string, find_two, parameter);
    int result_three = String_Utils_ends_with(string, null_string, parameter);
    if (result_one != 1 || result_two != 1 || result_three != 0) {
        assert(0);
    }
    printf("Passed test: Ends_With!\n");
    free(string);
    free(find_one);
    free(find_two);
}

void testString_Utils_equals() {
    char* string_one = "Check to see if this equals another string!";
    char* string_two = "CHECK TO SEE IF THIS EQUALS ANOTHER STRING!";
    char *null_string = NULL;
    int parameter_one = IGNORE_CASE;
    int parameter_two = NONE;
    int result_one = String_Utils_equals(string_one, string_two, parameter_one);
    int result_two = String_Utils_equals(string_one, string_two, parameter_two);
    int result_three = String_Utils_equals(string_one, null_string, parameter_one);
    if (result_one != 1 || result_two != 0 || result_three != 0) {
        assert(0);
    }
    printf("Passed test: Equals!\n");
    free(string_one);
    free(string_two);
}

void testString_Utils_free_array() {
   // char** array = { "Hello World", "How are you today?", "BEAUTIFUL I KNOW RIGHT!", "MWAHAHA boring debug" };
   // size_t size;
   // String_Utils_free_array(array, size);
    // Impossible to really test this in CUnit, the array's contents are freeds but the pointer is now undefined, not NULL.
    // Segfault = FAIL
}

void testString_Utils_from() {
    char* string = "Please get everything past here: I am an idiot!";
    char *null_string = NULL;
    unsigned int index = 33;
    int parameter = NONE;
    char* result_one = String_Utils_from(string, index, parameter);
    char *result_two = String_Utils_from(null_string, 0, parameter);
    char *result_three = String_Utils_from(string, 1000000000, parameter); // Intentionally attempt to read WAY past bounds
    if (strcmp(result_one, "I am an idiot!") != 0 || result_two != NULL || strcmp(result_three, "!") != 0) {
        assert(0);
    }
    free(string);
    free(result_one);
    free(result_two);
    free(result_three);
    printf("Passed test: From!\n");
}

void testString_Utils_from_token() {
    char* string = "Please token above: BLAH BLAH BLAH USELESS INFO <Parse_Me>int:32;char*:'Hello World';void*:NULL;</Parse_Me> BLAH BLAH BLAH USELESS INFO!";
    char* delimiter = "<Parse_Me>";
    char *null_string = NULL;
    int parameter = NONE;
    char* result_one = String_Utils_from_token(string, delimiter, parameter);
    char *result_two = String_Utils_from_token(string, null_string, parameter);
    printf("Result_One: %s\nResult_Two: %s\n", result_one, result_two); 
    if (strcmp(result_one, "<Parse_Me>int:32;char*:'Hello World';void*:NULL;</Parse_Me> BLAH BLAH BLAH USELESS INFO!") != 0) {
        assert(0);
    }
    if(result_two != NULL) assert(0);
    printf("Passed test: From_Token!\n");
    free(string);
    free(delimiter);
    free(result_one);
    free(result_two);
}

void testString_Utils_get_bytes() {
    char* string = "Hello World";
    char *null_string = NULL;
    unsigned int* result_one = String_Utils_get_bytes(string);
    unsigned int *result_two = String_Utils_get_bytes(null_string);
    int i = 0;
    for(i; i < strlen(string); i++){
        if(string[i] != (unsigned char)result_one[i]) assert(0); // Converts byte back to char to check consistency.
    }
    if (result_two !=  NULL) {
        assert(0);
    }
    printf("Passed test: Get_Bytes!\n");
    free(string);
    free(result_one);
    free(result_two);
}

void testString_Utils_index_of() {
    char* string = "The person with the best smile goes to: 'Amanda, the Panda!'";
    char* token = "Amanda, the Panda";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    int result_one = String_Utils_index_of(string, token, parameter);
    int result_two = String_Utils_index_of(null_string, token, parameter);
    if (string[result_one] != 'A' || result_two != -1) {
        assert(0);
    }
    printf("Passed test: Index_Of!\n");
    free(string);
    free(token);
}

void testString_Utils_join() {
    char** array_of_strings = (char **) malloc(sizeof(char *) * 4);
    array_of_strings[0] = "One prison"; array_of_strings[1] = "One person";
    array_of_strings[2] = "One bond"; array_of_strings[3] = "One Power!";
    char **null_array = NULL;
    char *delimiter = ", ";
    size_t size = 4;
    int parameter = NONE;
    char* result_one = String_Utils_join(array_of_strings, delimiter, &size, parameter);
    char *result_two = String_Utils_join(null_array, delimiter, &size, parameter);
    if (strcmp(result_one, "One prison, One person, One bond, One Power!") != 0 || result_two != NULL) {
        assert(0);
    }
    printf("Passed test: Join!\n");
    int i = 0;
    for(i; i < size; i++) free(array_of_strings[i]);
    free(array_of_strings);
    free(delimiter);
    free(result_one);
    free(result_two);
}

void testString_Utils_length() {
    char* string = "Hello World, how are you today?";
    char *null_string = NULL;
    int result_one = String_Utils_length(string);
    int result_two = String_Utils_length(null_string);
    if (result_one != strlen(string) || result_two != 0) {
        assert(0);
    }
    printf("Passed test: Length!\n");
    free(string);
}

void testString_Utils_replace() {
    char* string = "Lololol I love my soul enough to bowl with a fruit cannoli dipped in ravioli";
    char old_char = 'o';
    char new_char = 'e';
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    char* result_one = String_Utils_replace(string, old_char, new_char, parameter);
    char *result_two = String_Utils_replace(null_string, old_char, new_char, parameter);
    if (strcmp(result_one, "Lelelel I leve my seul eneugh te bewl with a fruit canneli dipped in ravieli") != 0 || result_two != NULL) {
        assert(0);
    }
    printf("Passed test: Replace!\n");
    free(string);
    free(result_one);
    free(result_two);
}

void testString_Utils_reverse() {
    char* string = "stressed"; // Like I am having to test all of this shit, GAHHHH!!!
    //char *modify_this = String_Utils_copy("stressed", NONE); // Test changes to set method's recent change
    char *null_string = NULL; // And you! God dammit, same copy and pasted test for NULL over and over!
    int parameter = NONE;
    char* result_one = String_Utils_reverse(string, parameter);
    char *result_two = String_Utils_reverse(null_string, parameter);
    //String_Utils_reverse(modify_this, MODIFY);
    if (strcmp(result_one, "desserts") != 0) { // || strcmp(modify_this, "desserts") != 0 || result_two != NULL) {
        assert(0);
    }
    printf("Passed test: Reverse!\n");
    free(string);
    free(result_one);
    free(result_two);
}

void testString_Utils_set() {
    char* string_one = String_Utils_copy("", NONE); // Empty string, as I do not want to allocate any memory or have a segfault
    char* string_two = "Hello world, bloody beautiful day isn't it? Have fun while I'm stuck inside testing for hours, and hours! Bastard.";
    int parameter = NONE;
    char* result = String_Utils_set(&string_one, string_two, parameter);
    if (strcmp(string_one, string_two) != 0) {
        assert(0);
    }
    printf("Passed test: Set!\n");
    free(string_one);
    free(string_two);
    free(result);
}

void testString_Utils_split() {
    char* string = "How else, says the Guard, does one continue along the path of righteousness, except by destroying all that is not righteous?";
    char* delimiter = ",";
    size_t* size = malloc(sizeof(size_t));
    int parameter = NONE;
    char** result = String_Utils_split(string, delimiter, size, parameter);
    int i = 0;
    if (strcmp(result[0], "How else") != 0 || strcmp(result[1], " says the Guard") != 0 || strcmp(result[2], " does one continue along the path of righteousness") != 0
            || strcmp(result[3], " except by destroying all that is not righteous?") != 0) {
        assert(0);
    }
    printf("Passed test: Split!\n");
    free(string);
    free(delimiter);
    for(i; i < *size; i++) free(result[i]);
    free(result);
    free(size);
}

void testString_Utils_starts_with() {
    char* string = "Is it me? Who else would it be, fool!";
    char* find = "is it me?";
    int parameter = IGNORE_CASE;
    int result = String_Utils_starts_with(string, find, parameter);
    if (result != 1) {
        assert(0);
    }
    printf("Passed test: Starts_With!\n");
    free(string);
    free(find);
}

void testString_Utils_substring() { // Segfaults here!
    char* string = "Below me lies the dragon... not just any dragon, the dragon called *gasp* *chokes* *dies*"; // Got bored debugging
    unsigned int begin = 28;
    unsigned int end = 9001; // Out of bounds... or is it?
    int parameter = NONE;
    char* result = String_Utils_substring(string, begin, end, parameter);
    if (strcmp(result, "not just any dragon, the dragon called *gasp* *chokes* *dies*") != 0) {
        assert(0);
    }
    printf("Passed test: Substring!\n");
    free(string);
    free(result);
}

void testString_Utils_to_lowercase() {
    char* string = "HELLO WORLD";
    int parameter = NONE;
    char* result = String_Utils_to_lowercase(string, parameter);
    if (strcmp(result, "hello world") != 0) {
        assert(0);
    }
    printf("Passed test: To_Lowercase!\n");
    free(string);
    free(result);
}

void testString_Utils_to_uppercase() {
    char* string = "hello world";
    int parameter = NONE;
    char* result = String_Utils_to_uppercase(string, parameter);
    if (strcmp(result, "HELLO WORLD") != 0) {
        assert(0);
    }
    printf("Passed test: To_uppercase!\n");
    free(string);
    free(result);
}

void testString_Utils_trim() {
    char* string = "        asdadadasd      ";
    int parameter = NONE;
    char* result = String_Utils_trim(string, parameter);
    if (strcmp(result, "asdadadasd") != 0) { // This guy...
        assert(0);
    }
    printf("Passed test: Trim!\n");
    free(string);
    free(result);
}

int main(void){
    TEST_ALL_FUNCTIONS;
    return 0;
}
