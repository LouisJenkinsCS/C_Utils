/*
 * File:   String_Utils_Unit_Test_2.c
 * Author: theif519
 *
 * Created on Apr 13, 2015, 11:47:50 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

/*
 * CUnit Test Suite
 */
String_Utils *su;
int init_suite(void) {
    su = String_Utils_create();
    return 0;
}

int clean_suite(void) {
    free(su);
    return 0;
}

void testString_Utils_capitalize() {
    char* string = "hello World";
    char *null_string = NULL; // Tests null pointer as argument
    int parameter = NONE;
    char* result_one = su->capitalize(string, parameter);
    char* result_two = su->capitalize(null_string, parameter);
    if (strcmp(result_one, "Hello World") != 0) {
        CU_ASSERT(0);
    }
    if(result_two != NULL) CU_ASSERT(0);
}

void testString_Utils_char_at() {
    char* string = "Hello World";
    char *null_string = NULL;
    unsigned int index_one = 100;
    unsigned int index_two = 5;
    char result_one = su->char_at(string, index_one);
    char result_two = su->char_at(string, index_two);
    char result_three = su->char_at(null_string, index_one);
    if (string[strlen(string)-1] != result_one) { // Tests with out of bounds index
        CU_ASSERT(0);
    }
    if(string[5] != result_two) CU_ASSERT(0); // Tests in-bounds index
    if(result_three != '\0') CU_ASSERT(0);
}

void testString_Utils_compare() {
    char* string_one = "Hello World";
    char* string_two = "Hello_World";
    char *string_three = "Hello WorlD";
    char *null_string = NULL;
    int parameters = IGNORE_CASE;
    int result_one = su->compare(string_one, string_two, parameters);
    int result_two = su->compare(null_string, string_two, parameters);
    int result_three = su->compare(string_one, string_one, parameters);
    int result_four = su->compare(string_one, string_three, parameters);
    if (result_one == 0) {
        CU_ASSERT(0);
    }
    if(result_two != -1) CU_ASSERT(0);
    if(result_three != 0) CU_ASSERT(0);
    if(result_four != 0) CU_ASSERT(0);
}

void testString_Utils_concat() {
    char* string_one = "Hello ";
    char* string_two = "World";
    char *string_three = su->copy("", NONE); // In order to change this value, a copy of the string is returned
    char *null_string = NULL;
    int parameter_one = NONE;
    int parameter_two = MODIFY;
    char* result_one = su->concat(string_one, string_two, parameter_one);
    char *result_two = su->concat(string_one, null_string, parameter_one);
    su->concat(string_three, result_one, parameter_two); // Modifies string_three
    if (strcmp(result_one, "Hello World") != 0) {
        CU_ASSERT(0);
    }
    if(result_two != NULL) CU_ASSERT(0);
    if(strcmp(result_one, string_three) != 0) CU_ASSERT(0);
}

void testString_Utils_contains() {
    char* string = "Hello World, the weather is nice today, isn't it?";
    char* search = "the";
    char *null_string = NULL;
    int parameters = IGNORE_CASE;
    int result_one = su->contains(string, search, parameters);
    int result_two = su->contains(string, null_string, parameters);
    if (result_one != 1) {
        CU_ASSERT(0);
    }
    if(result_two != 0) CU_ASSERT(0);
}

void testString_Utils_copy() {
    char* string = "Hello World";
    char *palindrome_string = "racecar";
    char *special_string = "stressed";
    char *null_string = NULL;
    int parameter = NONE;
    int reverse = REVERSE;
    char* result_one = su->copy(string, parameter);
    char *result_two = su->copy(palindrome_string, reverse); // If anything but the same string is returned, its bad
    char *result_three = su->copy(special_string, reverse); // You'll see
    char *result_four = su->copy(null_string, parameter);
    if (strcmp(string, result_one) != 0) {
        CU_ASSERT(0);
    }
    if(strcmp(palindrome_string, result_two) != 0) CU_ASSERT(0);
    if(strcmp(result_three, "desserts") != 0) CU_ASSERT(0); // Special string indeed!
    if(result_four != NULL) CU_ASSERT(0);
}

void testString_Utils_count() {
    char* string = "What is the meaning of the word 'the', when there is the person in the mirror staring back at the recipient?";
    char* delimiter = "the";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    int result_one = su->count(string, delimiter, parameter);
    int result_two = su->count(null_string, delimiter, parameter);
    if (result_one != 7) { // there has the word 'the' in it, no spaces
        CU_ASSERT(0);
    }
    if(result_two != 0) CU_ASSERT(0);
}

void testString_Utils_create() {
    String_Utils* result = String_Utils_create();
    if (result == NULL) {
        CU_ASSERT(0);
    }
}

void testString_Utils_ends_with() {
    char* string = "Catch the end of this string";
    char* find_one = "string";
    char *find_two = "END OF THIS STRING";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    int result_one = su->ends_with(string, find_one, parameter);
    int result_two = su->ends_with(string, find_two, parameter);
    int result_three = su->ends_with(string, null_string, parameter);
    if (result_one != 1 || result_two != 1 || result_three != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_equals() {
    char* string_one = "Check to see if this equals another string!";
    char* string_two = "CHECK TO SEE IF THIS EQUALS ANOTHER STRING!";
    char *null_string = NULL;
    int parameter_one = IGNORE_CASE;
    int parameter_two = NONE;
    int result_one = su->equals(string_one, string_two, parameter_one);
    int result_two = su->equals(string_one, string_two, parameter_two);
    int result_three = su->equals(string_one, null_string, parameter_one);
    if (result_one != 1 || result_two != 0 || result_three != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_free_array() {
   // char** array = { "Hello World", "How are you today?", "BEAUTIFUL I KNOW RIGHT!", "MWAHAHA boring debug" };
    //size_t size;
    //su->free_array(array, size);
    // Impossible to really test this in CUnit, the array's contents are freeds but the pointer is now undefined, not NULL.
    // Segfault = FAIL
}

void testString_Utils_from() {
    char* string = "Please get everything past here: I am an idiot!";
    char *null_string = NULL;
    unsigned int index = 33;
    int parameter = NONE;
    char* result_one = su->from(string, index, parameter);
    char *result_two = su->from(null_string, 0, parameter);
    char *result_three = su->from(string, 1000000000, parameter); // Intentionally attempt to read WAY past bounds
    if (strcmp(result_one, "I am an idiot!") != 0 || result_two != NULL || strcmp(result_three, "!") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_from_token() {
    char* string = "Please token above: BLAH BLAH BLAH USELESS INFO <Parse_Me>int:32;char*:'Hello World';void*:NULL;</Parse_Me> BLAH BLAH BLAH USELESS INFO!";
    char* delimiter = "<Parse_Me>";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    char* result_one = su->from_token(string, delimiter, parameter);
    char *result_two = su->from_token(string, null_string, parameter);
    if (strcmp(result_one, "<Parse_Me>int:32;char*:'Hello World';void*:NULL;</Parse_Me> BLAH BLAH BLAH USELESS INFO!") != 0) {
        CU_ASSERT(0);
    }
    if(result_two != NULL) CU_ASSERT(0);
}

void testString_Utils_get_bytes() {
    char* string = "Hello World";
    char *null_string = NULL;
    unsigned int* result_one = su->get_bytes(string);
    unsigned int *result_two = su->get_bytes(null_string);
    int i = 0;
    for(i; i < strlen(string); i++){
        if(string[i] != (unsigned char)result_one[i]) CU_ASSERT(0); // Converts byte back to char to check consistency.
    }
    if (result_two !=  NULL) {
        CU_ASSERT(0);
    }
}

void testString_Utils_index_of() {
    char* string = "The person with the best smile goes to: 'Amanda, the Panda!'";
    char* token = "Amanda, the Panda";
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    int result_one = su->index_of(string, token, parameter);
    int result_two = su->index_of(null_string, token, parameter);
    if (string[result_one] != 'A' || result_two != -1) {
        CU_ASSERT(0);
    }
}

void testString_Utils_join() {
    char** array_of_strings = malloc(sizeof(char *) * 4);
    array_of_strings[0] = "One prison"; array_of_strings[1] = "One person";
    array_of_strings[2] = "One bond"; array_of_strings[3] = "One Power!";
    char **null_array = NULL;
    char *delimiter = ", ";
    size_t size = 4;
    int parameter = NONE;
    char* result_one = su->join(array_of_strings, delimiter, &size, parameter);
    char *result_two = su->join(null_array, delimiter, &size, parameter);
    if (strcmp(result_one, "One prison, One person, One bond, One Power!") != 0 || result_two != NULL) {
        CU_ASSERT(0);
    }
}

void testString_Utils_length() {
    char* string = "Hello World, how are you today?";
    char *null_string = NULL;
    int result_one = su->length(string);
    int result_two = su->length(null_string);
    if (result_one != strlen(string) || result_two != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_replace() {
    char* string = "Lololol I love my soul enough to bowl with a fruit cannoli dipped in ravioli";
    char old_char = 'o';
    char new_char = 'e';
    char *null_string = NULL;
    int parameter = IGNORE_CASE;
    char* result_one = su->replace(string, old_char, new_char, parameter);
    char *result_two = su->replace(null_string, old_char, new_char, parameter);
    if (strcmp(result_one, "Lelelel I leve my seul eneugh te bewl with a fruit canneli dipped in ravieli") != 0 || result_two != NULL) {
        CU_ASSERT(0);
    }
}

void testString_Utils_reverse() {
    char* string = "stressed"; // Like I am having to test all of this shit, GAHHHH!!!
    char *modify_this = su->copy("stressed", NONE); // Test changes to set method's recent change
    char *null_string = NULL; // And you! God dammit, same copy and pasted test for NULL over and over!
    int parameter = NONE;
    char* result_one = su->reverse(string, parameter);
    char *result_two = su->reverse(null_string, parameter);
    su->reverse(modify_this, MODIFY);
    if (strcmp(result_one, "desserts") != 0 || strcmp(modify_this, "desserts") != 0 || result_two != NULL) {
        CU_ASSERT(0);
    }
}

void testString_Utils_set() {
    char* string_one = su->copy("", NONE); // Empty string, as I do not want to allocate any memory or have a segfault
    char* string_two = "Hello world, bloody beautiful day isn't it? Have fun while I'm stuck inside testing for hours, and hours! Bastard.";
    int parameter = NONE;
    char* result = su->set(&string_one, string_two, parameter);
    if (strcmp(string_one, string_two) != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_split() {
    char* string = "How else, says the Guard, does one continue along the path of righteousness, except by destroying all that is not righteous?";
    char* delimiter = ",";
    size_t* size = malloc(sizeof(size_t));
    int parameter = NONE;
    char** result = su->split(string, delimiter, size, parameter);
    int i = 0;
    if (strcmp(result[0], "How else") != 0 || strcmp(result[1], " says the Guard") != 0 || strcmp(result[2], " does one continue along the path of righteousness") != 0
            || strcmp(result[3], " except by destroying all that is not righteous?") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_starts_with() {
    char* string = "Is it me? Who else would it be, fool!";
    char* find = "is it me?";
    int parameter = IGNORE_CASE;
    int result = su->starts_with(string, find, parameter);
    if (result != 1) {
        CU_ASSERT(0);
    }
}

void testString_Utils_substring() {
    char* string = "Below me lies the dragon... not just any dragon, the dragon called *gasp* *chokes* *dies*"; // Got bored debugging
    unsigned int begin = 28;
    unsigned int end = 9001; // Out of bounds... or is it?
    int parameter = NONE;
    char* result = su->substring(string, begin, end, parameter);
    if (strcmp(result, "not just any dragon, the dragon called *gasp* *chokes* *dies*") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_to_lowercase() {
    char* string = "HELLO WORLD";
    int parameter = NONE;
    char* result = su->to_lowercase(string, parameter);
    if (strcmp(result, "hello world") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_to_uppercase() {
    char* string = "hello world";
    int parameter = NONE;
    char* result = su->to_uppercase(string, parameter);
    if (strcmp(result, "HELLO WORLD") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_trim() {
    char* string = "        asdadadasd      ";
    int parameter = NONE;
    char* result = su->trim(string, parameter);
    if (strcmp(result, "asdadadasd") != 0) { // This guy...
        CU_ASSERT(0);
    }
}

int main() {
    CU_pSuite pSuite = NULL;
    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();
    /* Add a suite to the registry */
    pSuite = CU_add_suite("String_Utils_Unit_Test_Struct_Callbacks", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "testString_Utils_capitalize", testString_Utils_capitalize)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_char_at", testString_Utils_char_at)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_compare", testString_Utils_compare)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_concat", testString_Utils_concat)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_contains", testString_Utils_contains)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_copy", testString_Utils_copy)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_count", testString_Utils_count)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_create", testString_Utils_create)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_ends_with", testString_Utils_ends_with)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_equals", testString_Utils_equals)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_free_array", testString_Utils_free_array)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_from", testString_Utils_from)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_from_token", testString_Utils_from_token)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_get_bytes", testString_Utils_get_bytes)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_index_of", testString_Utils_index_of)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_join", testString_Utils_join)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_length", testString_Utils_length)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_replace", testString_Utils_replace)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_reverse", testString_Utils_reverse)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_set", testString_Utils_set)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_split", testString_Utils_split)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_starts_with", testString_Utils_starts_with)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_substring", testString_Utils_substring)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_to_lowercase", testString_Utils_to_lowercase)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_to_uppercase", testString_Utils_to_uppercase)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_trim", testString_Utils_trim))) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
