#include "String_Utils.h"
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



char *String_Utils_concat(char **string_one, const char *string_two, int parameter) {
    assert(string_one);
    assert(*string_one);
    assert(string_two);
    char *temp;
    asprintf(&temp, "%s%s", *string_one, string_two);
    if (SELECTED(parameter, SU_MODIFY)) {
        String_Utils_set(string_one, temp);
        free(temp);
        return *string_one;
    } else return temp;
}


int String_Utils_compare(const char *string_one, const char *string_two, int parameter) {
    assert(string_one);
    assert(string_two);
    int result = 0;
    if(SELECTED(parameter, SU_IGNORE_CASE)){
        char *temp_string_one = String_Utils_to_lowercase(&string_one, SU_NONE);
        char *temp_string_two = String_Utils_to_lowercase(&string_two, SU_NONE);
        result = strcmp(temp_string_one, temp_string_two);
        free(temp_string_one);
        free(temp_string_two);
    } else result = strcmp(string_one, string_two);
    return result;
}

char String_Utils_char_at(const char *string, unsigned int index) {
    assert(string);
    return string[index > strlen(string) - 1 ? strlen(string) - 1 : index];
}

int String_Utils_contains(const char *string, const char *search, int parameter) { 
    assert(string);
    assert(search);
    int result = 0;
    if(SELECTED(parameter, SU_IGNORE_CASE)){
        char *temp_string = String_Utils_to_lowercase(&string, SU_NONE);
        char *temp_search = String_Utils_to_lowercase(&search, SU_NONE);
        result = strstr(temp_string, temp_search) == NULL ? 0 : 1;
        free(temp_string);
        free(temp_search);
    } else result = strstr(string, search) == NULL ? 0 : 1;
    return result;
}

char *String_Utils_to_lowercase(char **string, int parameter) {
    assert(string);
    assert(*string);
    int length = strlen(*string);
    char *temp = malloc(length + 1);
    int i = 0;
    while((*string)[i]) temp[i] = tolower((*string)[i++]);
    temp[length] = '\0';
    if (SELECTED(parameter, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_to_uppercase(char **string, int parameter) {
    assert(string);
    assert(*string);
    char *temp = malloc(strlen(*string) + 1);
    int i = 0;
    while((*string)[i]) temp[i] = toupper((*string)[i++]);
    temp[length] = '\0';
    if (SELECTED(parameter, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

unsigned int *String_Utils_get_bytes(const char *string) {
    // TODO: Implement a way to compress the string, rather than uselessly returning
    // an array of chars returned as an array of unsigned ints.
    return NULL;
}

int String_Utils_equals(const char *string_one, const char *string_two, int parameter) {
    assert(string_one);
    assert(string_two);
    return String_Utils_compare(string_one, string_two, parameter) == 0 ? 1 : 0;
}

char *String_Utils_from(char **string, unsigned int index, int parameter) {
    assert(string);
    assert(*string);
    int length = strlen(*string);
    int i = index > (length - 1) ? length - 1 : index;
    int j = 0;
    char *temp = malloc((length - i) + 1);
    while((*string)[i]) temp[j++] = (*string)[i++];
    temp[++j] = '\0'; 
    if (SELECTED(parameter, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_from_token(char **string, const char *substring, int parameter) {
    assert(string);
    assert(*string);
    assert(substring);
    char *temp_string = NULL;
    char *temp_substring = NULL;
    char *temp = NULL;
    if(SELECTED(parameter, SU_IGNORE_CASE)){
        temp_string = String_Utils_to_lowercase(*string, SU_NONE);
        temp_substring = String_Utils_to_lowercase(substring, SU_NONE);
    } else {
        temp_string = *string;
        temp_substring = substring;
    }
    *temp_string = strstr(*temp_string, temp_substring);
    if(!temp_string) return NULL;
    if(SELECTED(parameter, SU_LAST)){
        while(temp_string) {
            temp_string = strstr(temp_string, temp_substring);
            if(temp_string && strlen(temp) > strlen(substring)) {
                temp = temp_string;
                temp_string += strlen(substring);
            } else break;
        }
    }
    if(SELECTED(parameter, SU_IGNORE_CASE)){
        temp = String_Utils_substring(string, strlen(*string) - strlen(temp), strlen(*string), SU_NONE);
        free(temp_string);
        free(temp_substring);
    }
    if(SELECTED(parameter, SU_MODIFY)){
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

// Not Thread Safe! Update later to use strtok_r for reentrant and thread-safe splitting.
char **String_Utils_split(const char *string, const char *delimiter, size_t *size) {
    assert(string);
    assert(delimiter);
    assert(size); 
    char **string_array = malloc(sizeof (char *));
    char *temp = NULL;
    char *temp_string = strdup(string);
    if(!(temp = strtok(temp_string, delimiter))){ 
        free(temp_string); 
        return NULL; 
    } 
    unsigned int index = 0;
    while(temp){
        if(index) string_array = realloc(string_array, (sizeof (char *) * (index + 1))); 
        string_array[index] = malloc(strlen(temp) + 1);
        strcpy(string_array[index++], temp);
        temp = strtok(NULL, delimiter);
    }
    *size = index;
    free(temp_string);
    return string_array;
}

char *String_Utils_set(char **string_one, const char *string_two) {
    assert(string_one);
    assert(*string_one);
    assert(string_two);
    free(*string_one); // Will crash on a string literal.
    *string_one = strdup(string_two);
    return *string_one;
}

char *String_Utils_concat_all(int parameter, size_t amount, char **string, ...) {
    assert(string);
    assert(*string);
    va_list args;
    int i = 0;
    va_start(args, string);
    char *final_string = strdup(*string);
    char *temp = NULL;
    for(;i<amount; i++){
        temp = va_arg(args, char *);
        final_string = String_Utils_concat(&final_string, temp, SU_MODIFY);
    }
    va_end(args);
    if (SELECTED(parameter, SU_MODIFY)) {
        String_Utils_set(string, final_string);
        free(final_string);
        return string;
    } else return final_string;
}

char *String_Utils_reverse(char **string, int parameter) {
    assert(string);
    assert(*string);
    int i = 0, length = strlen(*string), j = length - 1;
    char *temp = malloc(length + 1);
    for (;i < length; i++, j--) temp[i] = (*string)[j];
    temp[length] = '\0';
    if (SELECTED(parameter, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_replace(char **string, char old_char, char new_char, int parameter){
    assert(string);
    assert(*string);
    int length = strlen(*string);
    char *temp = malloc(length + 1);
    int i = 0;
    if(SELECTED(parameter, SU_IGNORE_CASE)){
        for(;i <= length; i++) temp[i] = ((*string)[i] == tolower(old_char)) ? new_char : (*string)[i];
    } else for(;i <= length; i++) temp[i] = ((*string)[i] == old_char) ? new_char: (*string)[i];
    if(SELECTED(parameter, SU_MODIFY)){
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
    for(i; i < size; i++){
        if(!temp) temp = strdup(array_of_strings[i]);
        else String_Utils_concat_all(SU_MODIFY, 2, &temp, delimiter, array_of_strings[i]);
    }
    return temp;
}

int String_Utils_starts_with(const char *string, const char *find, int parameter){
    assert(string);
    assert(find);
    int i = 0, lowercase = SELECTED(parameter, SU_IGNORE_CASE);
    for(;i < strlen(find); i++){ 
        if(lowercase && tolower(string[i]) != tolower(find[i]) return 0;
        else if(!lowercase && string[i] != find[i]) return 0;
    }
    return 1;
}

int String_Utils_ends_with(const char *string, const char *find, int parameter){
    assert(string);
    assert(find);
    int string_length = strlen(string), find_length = strlen(find), i = string_length - find_length, j = 0;
    int lowercase = SELECTED(parameter, SU_IGNORE_CASE);
    for(;i < string_length; i++, j++){
        if(lowercase && tolower(string[i]) != tolower(find[j])) return 0; 
        else if(!lowercase && string[i] != find[j]) return 0;
    }
    return 1;
}

char *String_Utils_capitalize(char **string, int parameter){
    assert(string);
    assert(*string);
    char *temp = strdup(*string);
    temp[0] = toupper(temp[0]);
    if(SELECTED(parameter, SU_MODIFY)) { String_Utils_set(string, temp); free(temp); return *string; }
    return temp;
}

char *String_Utils_trim(char **string, int parameter){
    assert(string);
    assert(*string);
    char *temp = NULL;
    int length = strlen(*string), i = 0, j = length - 1;
    for(;i < length; i++) if(!isspace((*string)[i])) break;
    for(;j > i ; j--) if(!isspace((*string)[j])) break;
    temp = String_Utils_substring(string, i, j, SU_NONE);
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    }
    return temp;
}

char *String_Utils_substring(char **string, unsigned int begin, unsigned int end, int parameter){
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
    if(SELECTED(parameter, MODIFY)) {
        String_Utils_set(string, temp);
        free(temp); 
        return *string; 
    }
    return temp;
}

int String_Utils_index_of(const char *string, const char *substring, int parameter){
    assert(string);
    assert(substring);
    char *temp = NULL;
    char *temp_string = SELECTED(parameter, SU_IGNORE_CASE) ? String_Utils_to_lowercase(&string, SU_NONE) : NULL;
    char *temp_substring = SELECTED(parameter, SU_IGNORE_CASE) ? String_Utils_to_lowercase(&substring, SU_NONE) : NULL;
    char *old_temp = NULL;
    temp = strstr(temp_string ? temp_string : string, temp_substring ? temp_substring : substring);
    if(!temp || strlen(temp) < strlen(substring)){
        free(temp_string);
        free(temp_substring);
        return 0;
    }
    if(SELECTED(parameter, SU_LAST)){
        temp += strlen(substring);
        while(temp) {
            temp = strstr(temp, temp_substring == NULL ? substring : temp_substring);
            if(temp_string && strlen(temp) > strlen(substring)) {
                old_temp = temp;
                temp += strlen(substring);
            } else break;
        }
        temp = old_temp;
    }
    free(temp_string);
    free(temp_substring);
    return strlen(string) - strlen(temp);
}

int String_Utils_count(const char *string, const char *substring, int parameter){
    assert(string);
    assert(substring);
    int count = 0;
    char *temp = NULL; 
    char *temp_ptr = NULL;
    temp = SELECTED(parameter, SU_IGNORE_CASE) ? String_Utils_to_lowercase(&string, SU_NONE) : string;
    temp_ptr = temp;
    while(temp = strstr(temp, substring)){
        count++;
        temp_ptr = temp;
        if(strlen(temp) < strlen(substring)) break;
        else temp += strlen(substring);
    }
    return count;
}

// Reimplement function to use strcasestr().
char *String_Utils_between(const char *string, const char *start, const char *end, int parameter){
    assert(string);
    assert(start);
    assert(end);
    char *temp = NULL; 
    char *new_temp = NULL;
    char *temp_string = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(string, NONE) : string;
    char *temp_start = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(start, NONE) : start;
    char *temp_end = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(end, NONE) : end; 
    temp  = strstr(temp_string, temp_start) + strlen(temp_start);
    if(temp == NULL) return NULL;
    size_t size_of_substring = strlen(temp) - (strlen(strstr(temp_string, temp_end)));
    size_t index_of_start = strlen(string) - strlen(temp);
    new_temp = String_Utils_substring(string, index_of_start, size_of_substring, NONE);
    free(temp_string);
    free(temp_start);
    free(temp_end);
    free(temp);
    return new_temp;
}

void String_Utils_destroy(char **string){
    free(*string);
}