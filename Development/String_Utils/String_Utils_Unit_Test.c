/*
 * File:   String_Utils_Unit_Test.c
 * Author: theif519
 *
 * Created on Apr 11, 2015, 12:20:24 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

/*
 * CUnit Test Suite
 */

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

void testString_Utils_char_at() {
    char* string = "Hello World";
    unsigned int index = 1;
    char result = String_Utils_char_at(string, index);
    if (result != string[index]) {
        CU_ASSERT(0);
    }
}

void testString_Utils_compare() {
    char* string_one = "Hello World";
    char* string_two = "HeLlO WorlD";
    int parameters = IGNORE_CASE;
    int result = String_Utils_compare(string_one, string_two, parameters);
    if (result != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_concat() {
    char* string_one = "Hello, how is your ";
    char* string_two = "day so far?";
    int parameters = NONE;
    char* result = String_Utils_concat(string_one, string_two, parameters);
    //printf("Concat test: %s\n", result);
    if (strcmp(result, "Hello, how is your day so far?") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_contains() {
    char* string = "Madam, I am Adam";
    char* search = "I am";
    int parameters = IGNORE_CASE;
    int result = String_Utils_contains(string, search, parameters);
    printf("Contains test result: %d\n", result);
    if (result != 1) {
        CU_ASSERT(0);
    }
}

void testString_Utils_copy() {
    char* string = "Copy this string!";
    char* result = String_Utils_copy(string);
    if (strcmp(string, result) != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_create() {
    String_Utils* result = String_Utils_create();
    if (result == NULL) {
        CU_ASSERT(0);
    }
}

void testString_Utils_equals() {
    char* string_one = "This String";
    char* string_two = "this String";
    int parameter = IGNORE_CASE;
    int result = String_Utils_equals(string_one, string_two, parameter);
    if (result != 1) {
        CU_ASSERT(0);
    }
}

void testString_Utils_from() {
    char* string = "Please continue from this: How are you doing?";
    unsigned int index = 27;
    int parameter = NONE;
    char* result = String_Utils_from(string, index, parameter);
    printf("From test results: %s", result);
    if (strcmp(result, "How are you doing?") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_from_token() { // Not implemented~!
    char* string;
    char* delimiter;
    int parameter;
    CU_ASSERT(0);
    return;
    char* result = String_Utils_from_token(string, delimiter, parameter);
    if (1) {
        CU_ASSERT(0);
    }
}

void testString_Utils_get_bytes() {
    char* string = "Convert me to bytes!";
    unsigned int* result = String_Utils_get_bytes(string);
    if (strcmp(string, (char *)result) != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_length() {
    char* string = "My length is 15";
    int result = String_Utils_length(string);
    if (result != strlen(string)) {
        CU_ASSERT(0);
    }
}

void testString_Utils_replace() {
    char* string = "Lelelelel";
    char old_char = 'e';
    char new_char = 'o';
    int parameter = IGNORE_CASE;
    char* result = String_Utils_replace(string, old_char, new_char, parameter);
    if (strcmp(result, "Lolololol") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_reverse() {
    char* string = "olleH";
    int parameter = NONE;
    char* result = String_Utils_reverse(string, parameter);
    if (strcmp(result, "Hello") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_set() { // Does not work as intended! May segment fault
    char* string_one;
    char* string_two;
    int parameter;
    char* result = String_Utils_set(string_one, string_two, parameter);
    if (1) {
        CU_ASSERT(0);
    }
}

void testString_Utils_split() {
    char* string = "Hello, my, name, is, Louis";
    char* delimiter = ",";
    size_t* size;
    int parameter = NONE;
    char** result = String_Utils_split(string, delimiter, size, parameter);
    if ((strcmp(result[0], "Hello") != 0) || (strcmp(result[1], "my") != 0) || (strcmp(result[2], "name") != 0)
            || (strcmp(result[3], "is") != 0) || (strcmp(result[4], "Louis") != 0)) {
        CU_ASSERT(0);
    }
}

void testString_Utils_to_lowercase() {
    char* string = "HELLO";
    int parameter = NONE;
    char* result = String_Utils_to_lowercase(string, parameter);
    if (strcmp(result, "hello") != 0) {
        CU_ASSERT(0);
    }
}

void testString_Utils_to_uppercase() {
    char* string = "hello";
    int parameter = NONE;
    char* result = String_Utils_to_uppercase(string, parameter);
    if (strcmp(result, "HELLO") != 0) {
        CU_ASSERT(0);
    }
}

int main() {
    CU_pSuite pSuite = NULL;

    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry */
    pSuite = CU_add_suite("String_Utils_Unit_Test", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "testString_Utils_char_at", testString_Utils_char_at)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_compare", testString_Utils_compare)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_concat", testString_Utils_concat)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_contains", testString_Utils_contains)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_copy", testString_Utils_copy)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_create", testString_Utils_create)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_equals", testString_Utils_equals)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_from", testString_Utils_from)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_from_token", testString_Utils_from_token)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_get_bytes", testString_Utils_get_bytes)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_length", testString_Utils_length)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_replace", testString_Utils_replace)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_reverse", testString_Utils_reverse)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_set", testString_Utils_set)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_split", testString_Utils_split)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_to_lowercase", testString_Utils_to_lowercase)) ||
            (NULL == CU_add_test(pSuite, "testString_Utils_to_uppercase", testString_Utils_to_uppercase))) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
