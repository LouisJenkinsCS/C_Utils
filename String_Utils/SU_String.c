#define _GNU_SOURCE
#include "SU_String.h"

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
    logger = MU_Logger_create("./String_Utils/Logs/SU_String.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
    MU_Logger_destroy(logger);
}

/*
 * String_Utils has had it's garbage_collector garbage removed from it. For now, even as an experiment, it wasn't
 * worth claiming it as stable when it was unstable in and of itself. The garbage collector may have worked, but it
 * literally did nothing of value except complicate code and even increase the program size by 3x. So, instead, I
 * opted to have another experiment, one which is already heavily tested. The macro TEMP uses the
 * cleanup attribute specific for GCC and Clang compilers, making it 100% dependent on them to compile, but
 * it's easy enough to remove either way. Besides, it accomplished what I want to do. 
 *
 * Of course in the future I most likely (99.999% sure) I'm going to be replacing it entirely from the library functions,
 * as it currently is impossible to use without it being dependent on GCC and POSIX as is. Most likely, it will be apart of the whole C_Utils
 * package. However, I'll go into what TEMP does. What TEMP does is it will call the assigned callback for cleanup when it
 * leaves the variable's scope, most likely a function, but could be inside of a loop if it was declared there. TEMP will basically just free 
 * the string without further ado, preventing any memory leaks and pretty much eliminated needs to call free yourself.
 */

char SU_String_char_at(const SU_String_t string, unsigned int index){
    MU_ARG_CHECK(logger, '\0', string, string && index <= strlen(string));
    return string[index];
}

bool SU_String_contains(const SU_String_t str, const SU_String_t substr, size_t len, bool ignore_case){ 
    MU_ARG_CHECK(logger, false, str, substr);
    size_t str_len = len ? len : strlen(str);
    size_t substr_len = strlen(substr);
    if(substr_len > str_len) return false;
    if(substr_len == str_len){
        if(ignore_case){
            return strcasecmp(str, substr) == 0;
        }
        return strcmp(str, substr) == 0;
    }
    size_t i = 0;
    for(; i < str_len - substr_len; i++){
        if(str[i] == substr[i]){
            if(ignore_case){
                if(strncasecmp(str, substr, substr_len) == 0) return true;
                continue;
            }
            if(strncmp(str, substr, substr_len) == 0) return true;
        }
    }
    return false;
}

SU_String_t SU_String_to_lowercase(SU_String_t string, size_t len){
    MU_ARG_CHECK(logger, NULL, string);
    size_t str_len = len ? len : strlen(string), i = 0;
    for(; i < str_len; i++){
        char c = string[i];
        if(c == '\0') break;
        // Since tolower takes an int, and returns an int, I ensure (un)signedness by casting.
        string[i] = (char)tolower((unsigned char)c);
    }
    return string;
}

SU_String_t SU_String_to_uppercase(SU_String_t string, size_t len){
    MU_ARG_CHECK(logger, NULL, string);
    size_t str_len = len ? len : strlen(string), i = 0;
    for(; i < str_len; i++){
        char c = string[i];
        if(c == '\0') break;
        // Since tolower takes an int, and returns an int, I ensure (un)signedness by casting.
        string[i] = (char)toupper((unsigned char)c);
    }
    return string;
}

bool SU_String_equals(const SU_String_t string_one, const SU_String_t string_two, size_t len, bool ignore_case){
    MU_ARG_CHECK(logger, false, string_one, string_two);
    size_t str_len = len ? len : strlen(string_one);
    bool is_equal = false;
    if(ignore_case){
        is_equal = strncasecmp(string_one, string_two, str_len) == 0;
    } else {
        is_equal = strncmp(string_one, string_two, str_len) == 0;
    }
    return is_equal;
}

// Not Thread Safe! Update later to use strtok_r for reentrant and thread-safe splitting.
SU_String_t *SU_String_split(const SU_String_t string, size_t len, const SU_String_t delimiter, size_t *size){
    MU_ARG_CHECK(logger, NULL, string, delimiter);
    size_t str_len = len ? len : strlen(string), num_strings = 0;
    char str_copy[str_len + 1];
    snprintf(str_copy, str_len, "%s", string);
    char **split_strings = malloc(sizeof(char *));
    if(!split_strings){
        MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
        goto error;
    }
    char *saveptr;
    char *curr_string = strtok_r(str_copy, delimiter, &saveptr);
    if(!curr_string){ 
        free(split_strings);
        return NULL; 
    } 
    while(curr_string){
        char **tmp = realloc(split_strings, (sizeof(char *) * (num_strings + 1)));
        if(!tmp){
            MU_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
            goto error;
        }
        split_strings = tmp;
        split_strings[num_strings] = malloc(strlen(curr_string) + 1);
        if(!split_strings[num_strings]){
            MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
            goto error;
        }
        sprintf(split_strings[num_strings++], "%s", curr_string);
        curr_string = strtok_r(NULL, delimiter, &saveptr);
    }
    *size = num_strings;
    return split_strings;

    error:
        if(split_strings){
            int i = 0;
            for(; i < num_strings; i++){
                free(split_strings[i]);
            }
        }
        return NULL;
}

SU_String_t SU_String_reverse(SU_String_t string, size_t len){
    MU_ARG_CHECK(logger, NULL, string);
    size_t str_len = len ? len : strlen(string);
    int i = 0, j = str_len - 1;
    for(; i < j; i++, j--){
        char c = string[i];
        string[i] = string[j];
        string[j] = c;
    }
    return string;
}

char *String_Utils_replace(char **string, char old_char, char new_char, int flags){
    assert(string);
    assert(*string);
    int length = strlen(*string);
    char *temp = malloc(length + 1);
    int i = 0;
    if(MU_FLAG_GET(flags, SU_IGNORE_CASE)){
        for(;i <= length; i++) temp[i] = ((*string)[i] == tolower(old_char)) ? new_char : (*string)[i];
    } else for(;i <= length; i++) temp[i] = ((*string)[i] == old_char) ? new_char: (*string)[i];
    if(MU_FLAG_GET(flags, SU_MODIFY)){
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_join(const char **array_of_strings, const char *delimiter, size_t size){
    assert(array_of_strings);
    assert(delimiter);
    char *temp = NULL;
    int i = 0;
    for(;i < size; i++){
        if(!temp) temp = strdup(array_of_strings[i]);
        else String_Utils_concat_all(SU_MODIFY, 2, &temp, delimiter, array_of_strings[i]);
    }
    return temp;
}

int String_Utils_starts_with(const char *string, const char *find, int flags){
    assert(string);
    assert(find);
    int i = 0, lowercase = MU_FLAG_GET(flags, SU_IGNORE_CASE);
    for(;i < strlen(find); i++){ 
        if(lowercase && tolower(string[i]) != tolower(find[i])) return 0;
        else if(!lowercase && string[i] != find[i]) return 0;
    }
    return 1;
}

int String_Utils_ends_with(const char *string, const char *find, int flags){
    assert(string);
    assert(find);
    int string_length = strlen(string), find_length = strlen(find), i = string_length - find_length, j = 0;
    int lowercase = MU_FLAG_GET(flags, SU_IGNORE_CASE);
    for(;i < string_length; i++, j++){
        if(lowercase && tolower(string[i]) != tolower(find[j])) return 0; 
        else if(!lowercase && string[i] != find[j]) return 0;
    }
    return 1;
}

char *String_Utils_capitalize(char **string, int flags){
    assert(string);
    assert(*string);
    char *temp = strdup(*string);
    temp[0] = toupper(temp[0]);
    if(MU_FLAG_GET(flags, SU_MODIFY)) { 
        String_Utils_set(string, temp); 
        free(temp); 
        return *string; 
    } else return temp;
}

char *String_Utils_trim(char **string, int flags){
    assert(string);
    assert(*string);
    char *temp = NULL;
    int length = strlen(*string), i = 0, j = length - 1;
    for(;i < length; i++) if(!isspace((*string)[i])) break;
    for(;j > i ; j--) if(!isspace((*string)[j])) break;
    temp = String_Utils_substring(string, i, j, SU_NONE);
    if(MU_FLAG_GET(flags, SU_MODIFY)){
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    }
    return temp;
}

char *String_Utils_substring(char **string, unsigned int begin, unsigned int end, int flags){
    assert(string);
    assert(*string);
    unsigned int new_begin = begin > end ? 0 : begin; // Bounds checking for begin
    unsigned int new_end = end >= strlen(*string) ? strlen(*string) - 1 : end; // Bounds checking for end.
    new_end = (new_begin > new_end) ? new_begin : new_end; // Bounds checking to prevent unsigned int overflow (Or else 4GB+ malloc).
    // I.E if new_end < new_begin, then new_end = new_begin - x. Size = (new_begin - x) - new_begin = -x. Then size = 4294967295 - x.
    size_t size = new_end - new_begin;
    char *temp = malloc(size + 2);
    memcpy(temp, *string + begin, size + 1);
    temp[size + 1] = '\0';
    if(MU_FLAG_GET(flags, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp); 
        return *string; 
    }
    return temp;
}

int String_Utils_index_of(const char *string, const char *substring, int flags){
    assert(string);
    assert(substring);
    char *temp = NULL;
    char *old_temp = NULL;
    temp = MU_FLAG_GET(flags, SU_IGNORE_CASE) ? strcasestr(string, substring) : strstr(string, substring);
    if(!temp || strlen(temp) < strlen(substring)) return 0;
    if(MU_FLAG_GET(flags, SU_LAST)){
        temp += strlen(substring);
        while(temp) {
            temp = MU_FLAG_GET(flags, SU_IGNORE_CASE) ? strcasestr(temp, substring) : strstr(temp, substring);
            if(temp && strlen(temp) > strlen(substring)) {
                old_temp = temp;
                temp += strlen(substring);
            } else break;
        }
        temp = old_temp;
    }
    return strlen(string) - strlen(temp);
}

int String_Utils_count(const char *string, const char *substring, int flags){
    assert(string);
    assert(substring);
    int count = 0;
    char *temp = string;
    while(temp = MU_FLAG_GET(flags, SU_IGNORE_CASE) ? strcasestr(temp, substring) : strstr(temp, substring)){
        count++;
        if(strlen(temp) < strlen(substring)) break;
        else temp += strlen(substring);
    }
    return count;
}

// Reimplement function to use strcasestr().
char *String_Utils_between(const char *string, const char *start, const char *end, int flags){
    assert(string);
    assert(start);
    assert(end);
    char *temp = NULL; 
    char *new_temp = NULL;
    temp  = (MU_FLAG_GET(flags, SU_IGNORE_CASE) ? strcasestr(string, start) : strstr(string, start));
    if(!temp) return NULL;
    temp += strlen(start);
    new_temp = MU_FLAG_GET(flags, SU_IGNORE_CASE) ? strcasestr(temp, end) : strstr(temp, end);
    if(!new_temp) return NULL;
    size_t size_of_substring = strlen(temp) - strlen(new_temp);
    size_t index_of_start = strlen(string) - strlen(temp);
    new_temp = String_Utils_substring(string, index_of_start, size_of_substring, SU_NONE);
    return new_temp;
}

void String_Utils_destroy(char **string){
    free(*string);
}
