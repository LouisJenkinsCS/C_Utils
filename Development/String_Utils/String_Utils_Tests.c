#include "String_Utils.h"
#include <string.h>
#define BUF_SIZE 50

void test_String_Utils_join(void){
    size_t iteration = 0;
    char **array = malloc(sizeof(char *));
    char *temp = malloc(BUF_SIZE);
    while(1)
    {
        array[iteration] = malloc(BUF_SIZE);
        memset(temp, 0, BUF_SIZE);
        fgets(temp, BUF_SIZE, stdin);
        if(String_Utils_equals(temp, "quit\n", IGNORE_CASE)) break;
        array[iteration] = String_Utils_copy(temp);
        iteration++;
        //printf("Current iteration: %d\n", iteration);
        array = realloc(array, sizeof(char *) * (iteration + 1));
        //printf("Size of Array: %d\nRecent addition to array: %s", sizeof(array), array[iteration - 1]);
    }
    printf("You typed the following:\n%s", String_Utils_join(array, &iteration, NONE));
    free(array);
    free(temp);
}

void test_String_Utils_starts_with_and_ends_with(void){
    char *string = "Hi, how are you? Hi";
    char *find = "Hi";
    int parameter = IGNORE_CASE;
    int result_one = String_Utils_starts_with(string, find, parameter);
    int result_two = String_Utils_ends_with(string, find, parameter);
    printf("result_one: %d\nresult_two: %d", result_one, result_two);
}

void test_String_Utils_Free_Array(void){
    char **null_array;
    size_t *size = malloc(sizeof(size_t));
    char **full_array = String_Utils_split("Hello, World, How, Are, You?", ",", size, IGNORE_CASE);
    String_Utils_free_array(null_array, 1);
    String_Utils_free_array(full_array, *size);
    full_array = NULL;
}

void test_String_Utils_substring(void){
    char *temp = String_Utils_substring(String_Utils_copy("Hello World"), 2, 8, NONE);
    printf("%s", temp);
}

void test_String_Utils_trim(void){
    char *temp = String_Utils_copy(" How you like me now? ");
    printf("%s", String_Utils_trim(temp, NONE));
}

void test_String_Utils_capitalize(void){
    char *temp = String_Utils_copy("how you like me now?");
    printf("%s", String_Utils_capitalize(temp, NONE));
}

int main(void){
    
    return 0;
}