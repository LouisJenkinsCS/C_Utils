#include <String_Utils.h>
#include <string.h>
#include <stdlib.h>
#include <Misc_Utils.h>
#define TEST(condition) MU_ASSERT_RETURN(condition, fp, 0)
#define TEST_EQUAL(string_one, string_two) MU_ASSERT_RETURN(strcmp(string_one, string_two) == 0, fp, 0)
#define PASSED(test) MU_LOG_INFO(fp, "Passed Test %s\n", test)
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

FILE *fp;

int testString_Utils_capitalize() {
    char *string TEMP = strdup("hello World");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_capitalize(&string, parameter_one);
    String_Utils_capitalize(&string, parameter_two);
    TEST_EQUAL(result_one, "Hello World");
    TEST_EQUAL(string, result_one);
    PASSED("Capitalize");
}

int testString_Utils_char_at() {
    const char *string = "Hello World";
    unsigned int index_one = 100;
    unsigned int index_two = 5;
    char result_one = String_Utils_char_at(string, index_one);
    char result_two = String_Utils_char_at(string, index_two);
    TEST(string[10] == result_one);
    TEST(string[5] == result_two);
    PASSED("Char_At");
}

int testString_Utils_compare() {
    const char *string_one = "Hello World";
    const char *string_two = "Hello_World";
    const char *string_three = "Hello WorlD";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_compare(string_one, string_two, parameter_two) != 0);
    TEST(String_Utils_compare(string_one, string_three, parameter_one) == 0);
    TEST(String_Utils_compare(string_one, string_three, parameter_two) != 0);
    PASSED("Compare"); 
}

int  testString_Utils_concat() {
    char *string_one = "Hello ";
    const char *string_two = "World";
    char *string_three TEMP = strdup("Modify this string: "); // In order to change this value, a copy of the string is returned;
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_concat(&string_one, string_two, SU_NONE);
    String_Utils_concat(&string_three, result_one, parameter_two); // Modifies string_three
    TEST_EQUAL(result_one, "Hello World");
    TEST_EQUAL("Modify this string: Hello World", string_three);
    PASSED("Concat");
}

int  testString_Utils_contains() {
    const char *string = "Hello World, the weather is nice today, isn't it?";
    const char *search = "The";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_contains(string, search, parameter_one) == 1);
    TEST(String_Utils_contains(string, search, parameter_two) == 0);
    PASSED("Contains");
}


int  testString_Utils_count() {
    const char *string = "What is the meaning of the word 'the', when there is the person in the mirror staring back at the recipient? The answer is unclear.";
    const char *delimiter = "the";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_count(string, delimiter, parameter_one) == 8);
    TEST(String_Utils_count(string, delimiter, parameter_two) == 1);
    PASSED("Count");
}

int  testString_Utils_ends_with() {
    const char *string = "Catch the end of this string";
    const char *find_one = "string";
    const char *find_two = "END OF THIS STRING";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_ends_with(string, find_one, parameter_one) == 1);
    TEST(String_Utils_ends_with(string, find_two, parameter_one) == 1);
    TEST(String_Utils_ends_with(string, find_two, parameter_two) == 0);
    PASSED("Ends_With");
}

int  testString_Utils_equals() {
    const char *string_one = "Check to see if this equals another string!";
    const char *string_two = "CHECK TO SEE IF THIS EQUALS ANOTHER STRING!";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_equals(string_one, string_two, parameter_one) == 1);
    TEST(String_Utils_equals(string_one, string_two, parameter_two) == 0);
    PASSED("Equals");
}

int  testString_Utils_from() {
    char *string = "Please get everything past here: I am an idiot!";
    char *mutable_string TEMP = strdup("Modify this!");
    unsigned int index_one = 33;
    unsigned int index_two = 9001; // lol
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_from(&string, index_one, parameter_one);
    char *result_two TEMP = String_Utils_from(&string, index_two, parameter_one);
    String_Utils_from(&mutable_string, 7, parameter_two); // Magic number 7 is where "this!" starts.
    TEST_EQUAL(result_one, "I am an idiot!");
    TEST_EQUAL(result_two, "!");
    TEST_EQUAL(mutable_string, "this!");
    PASSED("From");
}

int  testString_Utils_from_token() {
    MU_LOG_INFO(fp, "Skipping From Token!\n");
    return 0;
    char *string TEMP = strdup("Please token above: BLAH BLAH BLAH USELESS INFO <Parse_Me>int:32;char*:'Hello World';void*:NULL;<Parse_Me> BLAH BLAH BLAH USELESS INFO!");
    const char *delimiter = "<Parse_Me>";
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    int parameter_three = SU_LAST;
    char *result_one TEMP = String_Utils_from_token(&string, delimiter, parameter_one);
    char *result_two TEMP = String_Utils_from_token(&string, delimiter, parameter_three);
    String_Utils_from_token(&string, delimiter, parameter_two);
    TEST_EQUAL(result_one, "<Parse_Me>int:32;char*:'Hello World';void*:NULL;<Parse_Me> BLAH BLAH BLAH USELESS INFO!");
    //TEST_EQUAL(result_two, "<Parse_Me> BLAH BLAH BLAH USELESS INFO!");
    TEST_EQUAL(string, result_one);
    PASSED("From_Token");
}

int  testString_Utils_index_of() {
    const char *string = "The person with the best smile goes to: 'Amanda, the Panda!'";
    const char *token = "AMANDA, the Panda";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    int result_one = String_Utils_index_of(string, token, parameter_one);
    TEST(string[String_Utils_index_of(string, token, parameter_one)] == 'A'); // Should be first character of token.
    TEST(string[String_Utils_index_of(string, token, parameter_two)] == 'T'); // It should be the very first index of the string.
    PASSED("Index_Of");
}

int  testString_Utils_join() {
    const char** array_of_strings = malloc(sizeof(char *) * 4);
    array_of_strings[0] = "One prison"; array_of_strings[1] = "One person";
    array_of_strings[2] = "One bond"; array_of_strings[3] = "One Power!";
    char *delimiter = ", ";
    size_t size = 4;
    char *result_one TEMP = String_Utils_join(array_of_strings, delimiter, size);
    TEST_EQUAL(result_one, "One prison, One person, One bond, One Power!");
    free(array_of_strings);
    PASSED("Join");
}

int  testString_Utils_replace() {
    char *string TEMP = strdup("Lololol I love my soul enough to bowl with a fruit cannoli dipped in ravioli");
    char old_char = 'O';
    char new_char = 'e';
    int parameter_one = SU_NONE;
    int parameter_two = SU_IGNORE_CASE;
    int parameter_three = SU_MODIFY | SU_IGNORE_CASE; // Multiple parameters
    char *result_one TEMP = String_Utils_replace(&string, old_char, new_char, parameter_one);
    char *result_two TEMP = String_Utils_replace(&string, old_char, new_char, parameter_two);
    String_Utils_replace(&string, old_char, new_char, parameter_three);
    TEST_EQUAL(result_one, "Lololol I love my soul enough to bowl with a fruit cannoli dipped in ravioli");
    TEST_EQUAL(result_two, "Lelelel I leve my seul eneugh te bewl with a fruit canneli dipped in ravieli");
    TEST_EQUAL(string, result_two);
    PASSED("Replaced");
}

int  testString_Utils_reverse() {
    char *string TEMP = strdup("stressed");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result_one TEMP = String_Utils_reverse(&string, parameter_one);
    String_Utils_reverse(&string, parameter_two);
    TEST_EQUAL(result_one, "desserts");
    TEST_EQUAL(string, result_one);
    PASSED("Reverse");
}

int  testString_Utils_set() {
    char *string_one TEMP = strdup("SU_MODIFY THIS!");
    const char *string_two = "Hello world, bloody beautiful day isn't it? Have fun while I'm stuck inside testing for hours, and hours! Bastard.";
    String_Utils_set(&string_one, string_two);
    TEST_EQUAL(string_one, string_two);
    PASSED("Set");
}

int  testString_Utils_split() {
    const char *string = "How else, says the Guard, does one continue along the path of righteousness, except by destroying all that is not righteous?";
    const char *delimiter = ",";
    size_t* size = malloc(sizeof(size_t));
    char** result = String_Utils_split(string, delimiter, size);
    int i = 0;
    TEST_EQUAL(result[0], "How else");
    TEST_EQUAL(result[1], " says the Guard");
    TEST_EQUAL(result[2], " does one continue along the path of righteousness");
    TEST_EQUAL(result[3], " except by destroying all that is not righteous?");
    for(i; i < *size; i++) free(result[i]);
    free(result);
    free(size);
    PASSED("Split");
}

int  testString_Utils_starts_with() {
    const char *string = "Is it me? Who else would it be, fool!";
    const char *find = "is it me?";
    int parameter_one = SU_IGNORE_CASE;
    int parameter_two = SU_NONE;
    TEST(String_Utils_starts_with(string, find, parameter_one) == 1);
    TEST(String_Utils_starts_with(string, find, parameter_two) == 0);
    PASSED("Starts_With");
}

int  testString_Utils_substring() {
    char *string TEMP = strdup("Below me lies the dragon... not just any dragon, the dragon called *gasp* *chokes* *dies*");
    unsigned int begin = 28;
    unsigned int end = 9001; // Out of bounds... or is it?
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_substring(&string, begin, end, parameter_one);
    String_Utils_substring(&string, begin, end, parameter_two);
    TEST_EQUAL(result, "not just any dragon, the dragon called *gasp* *chokes* *dies*");
    TEST_EQUAL(string, result);
    PASSED("Substring");
}

int  testString_Utils_to_lowercase() {
    char *string TEMP = strdup("HELLO WORLD");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_to_lowercase(&string, parameter_one);
    String_Utils_to_lowercase(&string, parameter_two);
    TEST_EQUAL(result, "hello world");
    TEST_EQUAL(string, result);
    PASSED("To_Lowercase");
}

int  testString_Utils_to_uppercase() {
    char *string TEMP = strdup("hello world");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_to_uppercase(&string, parameter_one);
    String_Utils_to_uppercase(&string, parameter_two);
    TEST_EQUAL(result, "HELLO WORLD");
    TEST_EQUAL(string, result);
    PASSED("To_Uppercase");
}

int testString_Utils_trim() {
    char *string TEMP = strdup("        asdadadasd      ");
    int parameter_one = SU_NONE;
    int parameter_two = SU_MODIFY;
    char *result TEMP = String_Utils_trim(&string, parameter_one);
    String_Utils_trim(&string, parameter_two);
    TEST_EQUAL(result, "asdadadasd");
    TEST_EQUAL(string, result);
    PASSED("Trim");
}

int main(void){
    fp = fopen("String_Utils_Test_Log.txt", "w");
    TEST_ALL_FUNCTIONS;
    PASSED("ALL");
    fclose(fp);
    return 0;
}
