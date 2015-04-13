#include "String_Utils.h"
#include <string.h>
#define BUF_SIZE 50
#define TEST_ALL_FUNCTIONS do { \
        test_String_Utils_count();\
        printf("\nPassed Test: Count!\n");\
        test_String_Utils_capitalize();\
        printf("\nPassed Test: Capitalize!\n");\
        test_String_Utils_index_of();\
        printf("\nPassed Test: Index_Of!\n");\
        test_String_Utils_join();\
        printf("\nPassed Test: Join!\n");\
        test_String_Utils_starts_with_and_ends_with();\
        printf("\nPassed Test: Starts_with_and_ends_with!\n");\
        test_String_Utils_Free_Array();\
        printf("\nPassed Test: Free_Array!\n");\
        test_String_Utils_trim();\
        printf("\nPassed Test: Trim!\n");\
        printf("\nPassed All Tests! YAY!!!\n");\
        } while(0);

String_Utils *su;

void test_String_Utils_join(void){
    size_t iteration = 0;
    char **array = malloc(sizeof(char *));
    char *temp = malloc(BUF_SIZE);
    while(1)
    {
        array[iteration] = malloc(BUF_SIZE);
        memset(temp, 0, BUF_SIZE);
        fgets(temp, BUF_SIZE, stdin);
        if(su->equals(temp, "quit\n", IGNORE_CASE)) break;
        array[iteration] = su->copy(temp, NONE);
        iteration++;
        //printf("Current iteration: %d\n", iteration);
        array = realloc(array, sizeof(char *) * (iteration + 1));
        //printf("Size of Array: %d\nRecent addition to array: %s", sizeof(array), array[iteration - 1]);
    }
    //printf("You typed the following:\n%s", su->join(array, &iteration, NONE));
    free(array);
    free(temp);
}

void test_String_Utils_starts_with_and_ends_with(void){
    char *string = "Hi, how are you? Hi";
    char *find = "Hi";
    int parameter = IGNORE_CASE;
    int result_one = su->starts_with(string, find, parameter);
    int result_two = su->ends_with(string, find, parameter);
    //printf("result_one: %d\nresult_two: %d", result_one, result_two);
}

void test_String_Utils_Free_Array(void){
    char **null_array = NULL;
    size_t *size = malloc(sizeof(size_t));
    char **full_array = su->split("Hello, World, How, Are, You?", ",", size, IGNORE_CASE);
    su->free_array(null_array, 1);
    su->free_array(full_array, *size);
    full_array = NULL;
}

void test_String_Utils_substring(void){
    char *temp = su->substring(su->copy("Hello World", NONE), 2, 8, NONE);
   // printf("%s", temp);
}

void test_String_Utils_trim(void){
    char *temp = su->copy(" How you like me now? ", NONE);
    //printf("%s", su->trim(temp, NONE));
}

void test_String_Utils_capitalize(void){
    char *temp = su->copy("how you like me now?", NONE);
    //printf("%s", su->capitalize(temp, NONE));
}

void test_String_Utils_index_of(void){
    char *temp = su->copy("Hello World", NONE);
    int result = su->index_of(temp, "World", NONE);
    //printf("Result: %d; String: %s\n", result, su->substring(temp, result, 0, NONE));
}

void test_String_Utils_count(void){
    VALIDATE_PTR_VOID(su);
    char *temp = su->copy("The the the wind is the endsthe ofthethethe the lololtheol", NONE);
    int result = su->count(temp, su->copy("the", NONE), IGNORE_CASE);
   // printf("The amount of times 'the' appears in the sentence: '%s' is %d\n", temp, result);
}
int main(void){
    su = String_Utils_create();
    TEST_ALL_FUNCTIONS;
    return 0;
}