#define _GNU_SOURCE
#include "String_Utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
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

static int is_selected(int mask, int flag){
    return mask & flag;
}

char *String_Utils_concat(char **string_one, const char *string_two, int flags) {
    assert(string_one);
    assert(*string_one);
    assert(string_two);
    char *temp;
    asprintf(&temp, "%s%s", *string_one, string_two);
    if (is_selected(flags, SU_MODIFY)) {
        String_Utils_set(string_one, temp);
        free(temp);
        return *string_one;
    } else return temp;
}


int String_Utils_compare(const char *string_one, const char *string_two, int flags) {
    assert(string_one);
    assert(string_two);
    return is_selected(flags, SU_IGNORE_CASE) ? strcasecmp(string_one, string_two) : strcmp(string_one, string_two);
}

char String_Utils_char_at(const char *string, unsigned int index) {
    assert(string);
    return string[index > strlen(string) - 1 ? strlen(string) - 1 : index];
}

int String_Utils_contains(const char *string, const char *search, int flags) { 
    assert(string);
    assert(search);
    return is_selected(flags, SU_IGNORE_CASE) ? (strcasestr(string, search) != NULL) : (strstr(string, search) != NULL) ;
}

char *String_Utils_to_lowercase(char **string, int flags) {
    assert(string);
    assert(*string);
    int length = strlen(*string);
    char *temp = malloc(length + 1);
    int i = 0;
    while((*string)[i]) temp[i] = tolower((*string)[i++]);
    temp[length] = '\0';
    if (is_selected(flags, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_to_uppercase(char **string, int flags) {
    assert(string);
    assert(*string);
    int length = strlen(*string);
    char *temp = malloc(length + 1);
    int i = 0;
    while((*string)[i]) temp[i] = toupper((*string)[i++]);
    temp[length] = '\0';
    if (is_selected(flags, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

int String_Utils_equals(const char *string_one, const char *string_two, int flags) {
    assert(string_one);
    assert(string_two);
    return String_Utils_compare(string_one, string_two, flags) == 0;
}

char *String_Utils_from(char **string, unsigned int index, int flags) {
    assert(string);
    assert(*string);
    int length = strlen(*string);
    int i = index > (length - 1) ? length - 1 : index;
    int j = 0;
    char *temp = malloc((length - i) + 1);
    while((*string)[i]) temp[j++] = (*string)[i++];
    temp[j] = '\0'; 
    if (is_selected(flags, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_from_token(char **string, const char *substring, int flags) {
    assert(string);
    assert(*string);
    assert(substring);
    char *temp = NULL;
    char *temp_string = is_selected(flags, SU_IGNORE_CASE) ? strcasestr(*string, substring) : strstr(*string, substring);
    if(!temp_string) return NULL;
    if(is_selected(flags, SU_LAST)){
        while(temp_string) {
            temp_string = is_selected(flags, SU_IGNORE_CASE) ? strcasestr(*string, substring) : strstr(*string, substring);
            if(temp_string && strlen(temp_string) > strlen(substring)) {
                temp = temp_string;
                temp_string += strlen(substring);
            } else break;
        }
        temp_string = temp;
    }
    if(is_selected(flags, SU_MODIFY)){
        String_Utils_set(string, temp_string);
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
    char *saveptr;
    char *temp_string = strdup(string);
    if(!(temp = strtok_r(temp_string, delimiter, &saveptr))){ 
        free(temp_string); 
        return NULL; 
    } 
    unsigned int index = 0;
    while(temp){
        if(index) string_array = realloc(string_array, (sizeof (char *) * (index + 1))); 
        string_array[index] = malloc(strlen(temp) + 1);
        strcpy(string_array[index++], temp);
        temp = strtok_r(NULL, delimiter, &saveptr);
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

char *String_Utils_concat_all(int flags, size_t amount, char **string, ...) {
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
    if (is_selected(flags, SU_MODIFY)) {
        String_Utils_set(string, final_string);
        free(final_string);
        return *string;
    } else return final_string;
}

char *String_Utils_reverse(char **string, int flags) {
    assert(string);
    assert(*string);
    int i = 0, length = strlen(*string), j = length - 1;
    char *temp = malloc(length + 1);
    for (;i < length; i++, j--) temp[i] = (*string)[j];
    temp[length] = '\0';
    if (is_selected(flags, SU_MODIFY)) {
        String_Utils_set(string, temp);
        free(temp);
        return *string;
    } else return temp;
}

char *String_Utils_replace(char **string, char old_char, char new_char, int flags){
    assert(string);
    assert(*string);
    int length = strlen(*string);
    char *temp = malloc(length + 1);
    int i = 0;
    if(is_selected(flags, SU_IGNORE_CASE)){
        for(;i <= length; i++) temp[i] = ((*string)[i] == tolower(old_char)) ? new_char : (*string)[i];
    } else for(;i <= length; i++) temp[i] = ((*string)[i] == old_char) ? new_char: (*string)[i];
    if(is_selected(flags, SU_MODIFY)){
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

int String_Utils_starts_with(const char *string, const char *find, int flags){
    assert(string);
    assert(find);
    int i = 0, lowercase = is_selected(flags, SU_IGNORE_CASE);
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
    int lowercase = is_selected(flags, SU_IGNORE_CASE);
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
    if(is_selected(flags, SU_MODIFY)) { 
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
    if(is_selected(flags, SU_MODIFY)){
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
    if(is_selected(flags, SU_MODIFY)) {
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
    temp = is_selected(flags, SU_IGNORE_CASE) ? strcasestr(string, substring) : strstr(string, substring);
    if(!temp || strlen(temp) < strlen(substring)) return 0;
    if(is_selected(flags, SU_LAST)){
        temp += strlen(substring);
        while(temp) {
            temp = is_selected(flags, SU_IGNORE_CASE) ? strcasestr(temp, substring) : strstr(temp, substring);
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
    while(temp = is_selected(flags, SU_IGNORE_CASE) ? strcasestr(temp, substring) : strstr(temp, substring)){
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
    temp  = (is_selected(flags, SU_IGNORE_CASE) ? strcasestr(string, start) : strstr(string, start));
    if(!temp) return NULL;
    temp += strlen(start);
    new_temp = is_selected(flags, SU_IGNORE_CASE) ? strcasestr(temp, end) : strstr(temp, end);
    if(!new_temp) return NULL;
    size_t size_of_substring = strlen(temp) - strlen(new_temp);
    size_t index_of_start = strlen(string) - strlen(temp);
    new_temp = String_Utils_substring(string, index_of_start, size_of_substring, SU_NONE);
    return new_temp;
}

void String_Utils_destroy(char **string){
    free(*string);
}