#include <SU_String.h>
#include <string.h>
#include <stdlib.h>
#include <MU_Logger.h>
#define TEST(condition, test) do { \
    if(!(condition)) FAILED(test); \
    MU_ASSERT_RETURN(condition, logger, 0, "Failed test %s!\n", test); \
} while(0)
#define TEST_EQUAL(string_one, string_two, test) do { \
    int cmp_result = 0; \
    if((cmp_result = strcmp(string_one, string_two)) != 0) FAILED(test); \
    MU_ASSERT_RETURN(strcmp(string_one, string_two) == 0, logger, 0); \
} while(0)
#define PASSED(test) do { \
	MU_LOG_INFO(logger, "Passed Test %s\n", test); \
	return 1; \
} while(0)
#define FAILED(test) MU_LOG_ERROR(logger, "Failed Test %s\n", test)
#define TEST_ALL_FUNCTIONS testString_Utils_capitalize();\
	testString_Utils_char_at();\
	testString_Utils_compare();\
	testString_Utils_concat();\
	testString_Utils_count();\
	testString_Utils_ends_with();\
	testString_Utils_equals();\
	testString_Utils_from();\
	testString_Utils_from_token();\
	testString_Utils_index_of();\
	testString_Utils_join();\
	testString_Utils_replace();\
	testString_Utils_reverse();\
	testString_Utils_set();\
	testString_Utils_split();\
	testString_Utils_starts_with();\
	testString_Utils_substring();\
	testString_Utils_to_lowercase();\
	testString_Utils_to_uppercase();\
	testString_Utils_trim()

MU_Logger_t *logger = NULL;

int testString_Utils_capitalize() {
    char test[] = "Capitalize";
    char *string TEMP = strdup("hello World");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_capitalize(&string, parameter_one);
    String_Utils_capitalize(&string, parameter_two);
    TEST_EQUAL(result_one, "Hello World", test);
    TEST_EQUAL(string, result_one, test);
    PASSED(test);
}

int testString_Utils_char_at() {
    char test[] = "Char_At";
    const char *string = "Hello World";
    unsigned int index_one = 100;
    unsigned int index_two = 5;
    char result_one = String_Utils_char_at(string, index_one);
    char result_two = String_Utils_char_at(string, index_two);
    TEST(string[10] == result_one, test);
    TEST(string[5] == result_two, test);
    PASSED(test);
}

int testString_Utils_compare() {
    char test[] = "Compare";
    const char *string_one = "Hello World";
    const char *string_two = "Hello_World";
    const char *string_three = "Hello WorlD";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_compare(string_one, string_two, parameter_two) != 0, test);
    TEST(String_Utils_compare(string_one, string_three, parameter_one) == 0, test);
    TEST(String_Utils_compare(string_one, string_three, parameter_two) != 0, test);
    PASSED(test); 
}

int  testString_Utils_concat() {
    char test[] = "Concat";
    char *string_one = "Hello ";
    const char *string_two = "World";
    char *string_three TEMP = strdup("Modify this string: "); // In order to change this value, a copy of the string is returned;
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_concat(&string_one, string_two, SU_NONE);
    String_Utils_concat(&string_three, result_one, parameter_two); // Modifies string_three
    TEST_EQUAL(result_one, "Hello World", test);
    TEST_EQUAL("Modify this string: Hello World", string_three, test);
    PASSED(test);
}

int  testString_Utils_contains() {
    char test[] = "Contains";
    const char *string = "Hello World, the weather is nice today, isn't it?";
    const char *search = "The";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_contains(string, search, parameter_one) == 1, test);
    TEST(String_Utils_contains(string, search, parameter_two) == 0, test);
    PASSED(test);
}


int  testString_Utils_count() {
    char test[] = "Count";
    const char *string = "What is the meaning of the word 'the', when there is the person in the mirror staring back at the recipient? The answer is unclear.";
    const char *delimiter = "The";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_count(string, delimiter, parameter_one) == 8, test);
    TEST(String_Utils_count(string, delimiter, parameter_two) == 1, test);
    PASSED(test);
}

int  testString_Utils_ends_with() {
    char test[] = "Ends_With";
    const char *string = "Catch the end of this string";
    const char *find_one = "string";
    const char *find_two = "END OF THIS STRING";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_ends_with(string, find_one, parameter_one) == 1, test);
    TEST(String_Utils_ends_with(string, find_two, parameter_one) == 1, test);
    TEST(String_Utils_ends_with(string, find_two, parameter_two) == 0, test);
    PASSED(test);
}

int  testString_Utils_equals() {
    char test[] = "Equals";
    const char *string_one = "Check to see if this equals another string!";
    const char *string_two = "CHECK TO SEE IF THIS EQUALS ANOTHER STRING!";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_equals(string_one, string_two, parameter_one) == 1, test);
    TEST(String_Utils_equals(string_one, string_two, parameter_two) == 0, test);
    PASSED(test);
}

int  testString_Utils_from() {
    char test[] = "From";
    char *string = "Please get everything past here: I am an idiot!";
    char *mutable_string TEMP = strdup("Modify this!");
    unsigned int index_one = 33;
    unsigned int index_two = 9001; // lol
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_from(&string, index_one, parameter_one);
    char *result_two TEMP = String_Utils_from(&string, index_two, parameter_one);
    String_Utils_from(&mutable_string, 7, parameter_two); // Magic number 7 is where "this!" starts.
    TEST_EQUAL(result_one, "I am an idiot!", test);
    TEST_EQUAL(result_two, "!", test);
    TEST_EQUAL(mutable_string, "this!", test);
    PASSED(test);
}

int  testString_Utils_from_token() {
    char test[] = "From_Token";
    char *string TEMP = strdup("Please token above: BLAH BLAH BLAH USELESS INFO <Parse_Me>int:32;char*:'Hello World';void*:NULL;<Parse_Me> BLAH BLAH BLAH USELESS INFO!");
    const char *delimiter = "<Parse_Me>";
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    int parameter_three = SU_LAST;
    char *result_one TEMP = String_Utils_from_token(&string, delimiter, parameter_one);
    char *result_two TEMP = String_Utils_from_token(&string, delimiter, parameter_three);
    String_Utils_from_token(&string, delimiter, parameter_two);
    TEST_EQUAL(result_one, "<Parse_Me>int:32;char*:'Hello World';void*:NULL;<Parse_Me> BLAH BLAH BLAH USELESS INFO!", test);
    TEST_EQUAL(result_two, "<Parse_Me> BLAH BLAH BLAH USELESS INFO!", test);
    TEST_EQUAL(string, result_one, test);
    PASSED(test);
}

int  testString_Utils_index_of() {
    char test[] = "Index_Of";
    const char *string = "The person with the best smile goes to: 'Amanda, the Panda!'";
    const char *token = "AMANDA, the Panda";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    int result_one = String_Utils_index_of(string, token, parameter_one);
    TEST(string[String_Utils_index_of(string, token, parameter_one)] == 'A', test); // Should be first character of token.
    TEST(string[String_Utils_index_of(string, token, parameter_two)] == 'T', test); // It should be the very first index of the string.
    PASSED(test);
}

int  testString_Utils_join() {
    char test[] = "Join";
    const char** array_of_strings = malloc(sizeof(char *) * 4);
    array_of_strings[0] = "One prison"; array_of_strings[1] = "One person";
    array_of_strings[2] = "One bond"; array_of_strings[3] = "One Power!";
    char *delimiter = ", ";
    size_t size = 4;
    char *result_one TEMP = String_Utils_join(array_of_strings, delimiter, size);
    TEST_EQUAL(result_one, "One prison, One person, One bond, One Power!", test);
    free(array_of_strings);
    PASSED(test);
}

int  testString_Utils_replace() {
    char test[] = "Replace";
    char *string TEMP = strdup("Lololol I love my soul enough to bowl with a fruit cannoli dipped in ravioli");
    char old_char = 'O';
    char new_char = 'e';
    int parameter_one = SU_NONE;
    int parameter_two = SU_IGNORE_CASE;
    int parameter_three = SU_MODIFY | SU_IGNORE_CASE; // Multiple parameters
    char *result_one TEMP = String_Utils_replace(&string, old_char, new_char, parameter_one);
    char *result_two TEMP = String_Utils_replace(&string, old_char, new_char, parameter_two);
    String_Utils_replace(&string, old_char, new_char, parameter_three);
    TEST_EQUAL(result_one, "Lololol I love my soul enough to bowl with a fruit cannoli dipped in ravioli", test);
    TEST_EQUAL(result_two, "Lelelel I leve my seul eneugh te bewl with a fruit canneli dipped in ravieli", test);
    TEST_EQUAL(string, result_two, test);
    PASSED(test);
}

int  testString_Utils_reverse() {
    char test[] = "Reverse";
    char *string TEMP = strdup("stressed");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_reverse(&string, parameter_one);
    String_Utils_reverse(&string, parameter_two);
    TEST_EQUAL(result_one, "desserts", test);
    TEST_EQUAL(string, result_one, test);
    PASSED(test);
}

int  testString_Utils_set() {
    char test[] = "Set";
    char *string_one TEMP = strdup("SU_MODIFY THIS!");
    const char *string_two = "Hello world, bloody beautiful day isn't it? Have fun while I'm stuck inside testing for hours, and hours! Bastard.";
    String_Utils_set(&string_one, string_two);
    TEST_EQUAL(string_one, string_two, test);
    PASSED(test);
}

int  testString_Utils_split() {
    char test[] = "Split";
    const char *string = "How else, says the Guard, does one continue along the path of righteousness, except by destroying all that is not righteous?";
    const char *delimiter = ",";
    size_t* size = malloc(sizeof(size_t));
    char** result = String_Utils_split(string, delimiter, size);
    int i = 0;
    TEST_EQUAL(result[0], "How else", test);
    TEST_EQUAL(result[1], " says the Guard", test);
    TEST_EQUAL(result[2], " does one continue along the path of righteousness", test);
    TEST_EQUAL(result[3], " except by destroying all that is not righteous?", test);
    for(;i < *size; i++) free(result[i]);
    free(result);
    free(size);
    PASSED(test);
}

int  testString_Utils_starts_with() {
    char test[] = "Starts_With";
    const char *string = "Is it me? Who else would it be, fool!";
    const char *find = "is it me?";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_starts_with(string, find, parameter_one) == 1, test);
    TEST(String_Utils_starts_with(string, find, parameter_two) == 0, test);
    PASSED(test);
}

int  testString_Utils_substring() {
    char test[] = "Substring";
    char *string TEMP = strdup("Below me lies the dragon... not just any dragon, the dragon called *gasp* *chokes* *dies*");
    unsigned int begin = 28;
    unsigned int end = 9001; // Out of bounds... or is it?
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_substring(&string, begin, end, parameter_one);
    String_Utils_substring(&string, begin, end, parameter_two);
    TEST_EQUAL(result, "not just any dragon, the dragon called *gasp* *chokes* *dies*", test);
    TEST_EQUAL(string, result, test);
    PASSED(test);
}

int  testString_Utils_to_lowercase() {
    char test[] = "To_Lowercase";
    char *string TEMP = strdup("HELLO WORLD");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_to_lowercase(&string, parameter_one);
    String_Utils_to_lowercase(&string, parameter_two);
    TEST_EQUAL(result, "hello world", test);
    TEST_EQUAL(string, result, test);
    PASSED(test);
}

int  testString_Utils_to_uppercase() {
    char test[] = "To_Uppercase";
    char *string TEMP = strdup("hello world");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_to_uppercase(&string, parameter_one);
    String_Utils_to_uppercase(&string, parameter_two);
    TEST_EQUAL(result, "HELLO WORLD", test);
    TEST_EQUAL(string, result, test);
    PASSED(test);
}

int testString_Utils_trim() {
    char test[] = "Trim";
    char *string TEMP = strdup("        asdadadasd      ");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_trim(&string, parameter_one);
    String_Utils_trim(&string, parameter_two);
    TEST_EQUAL(result, "asdadadasd", test);
    TEST_EQUAL(string, result, test);
    PASSED(test);
}

int main(void){
    MU_Logger_init(logger, "String_Utils_Test.log", "w", MU_ALL);
    MU_Timer_t *timer = MU_Timer_Init(1);
    TEST_ALL_FUNCTIONS;
    MU_LOG_INFO(logger, "All tests finished!\n");
    Timer_Stop(timer);
    char *total_time = Timer_To_String(timer);
    MU_LOG_INFO(logger, "Total Time: %s\n", total_time);
    Timer_Destroy(timer);
    free(total_time);
    MU_Logger_destroy(logger);
    return 0;
}
