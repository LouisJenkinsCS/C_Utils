#include <SU_String.h>
#include <string.h>
#include <stdlib.h>
#include <MU_Logger.h>
#include <MU_Test.h>

static MU_Logger_t *logger = NULL;

int main(void){
    logger = MU_Logger_create("./String_Utils/Logs/SU_String_Test.log", "w", MU_ALL);
    char *str = strdup("Hello World");
    SU_String_upper(str, 0);
    MU_TEST(strcmp(str, "HELLO WORLD") == 0, logger, "SU_String_upper");
    SU_String_lower(str, 5);
    MU_TEST(strcmp(str, "hello WORLD") == 0, logger,  "SU_String_lower");
    SU_String_reverse(str + 6, 0);
    MU_TEST(strcmp(str, "hello DLROW") == 0, logger, "SU_String_reverse");
    // Checking from offset of 3 to see if the string contains "dl" with case insensitive comparison.
    int index = SU_String_index_of(str + 3, "dl", 0, true);
    MU_TEST(index == 3, logger, "SU_String_index_of");
    char *substr = SU_String_substring(str, 0, 5);
    MU_TEST(strcmp(substr, "hello") == 0, logger, "SU_String_substring");
    free(substr);
    bool starts_with = SU_String_starts_with(str, "hello", true);
    MU_TEST(starts_with, logger, "SU_String_starts_with");
    bool ends_with = SU_String_ends_with(str, "dlrow", true);
    MU_TEST(ends_with, logger, "SU_String_ends_with");
    /// Check log file!
    char c = SU_String_char_at(str, 100);
    MU_TEST(c == '\0', logger, "SU_String_char_at");
    free(str);
    str = strdup("Worldly world of worldington is out of this world! Word to your world! Word to my world! Word to every world! WORLD!!!");
    bool contains = SU_String_contains(str, "world", 0, false);
    MU_TEST(contains, logger, "SU_String_contains");
    int count = SU_String_count(str, "World", 0, false);
    MU_TEST(count == 1, logger, "SU_String_count");
    size_t arr_size;
    char **arr = SU_String_split(str, "!", 0, &arr_size);
    MU_TEST(arr && arr_size && strcmp(arr[3], " Word to every world") == 0, logger, "SU_String_split");
    free(str);
    str = SU_String_join(arr, "\n", arr_size);
    MU_TEST(strcmp(str, "Worldly world of worldington is out of this world\n Word to your world\n Word to my world\n Word to every world\n WORLD") == 0, logger, "SU_String_join");
    char *new_str;
    SU_STRING_CONCAT_ALL(&new_str, "\n", arr[0], arr[1], arr[2], arr[3], arr[4]);
    MU_TEST(strcmp(str, new_str) == 0, logger, "SU_STRING_CONCAT_ALL");
    SU_String_replace(str, 'o', 'e', 0, true);
    MU_TEST(strcmp(str, "Werldly werld ef werldingten is eut ef this werld\n Werd te yeur werld\n Werd te my werld\n Werd te every werld\n WeRLD") == 0, logger, "SU_String_join");
    free(str);
    return EXIT_SUCCESS;
}
