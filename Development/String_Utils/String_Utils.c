#include "String_Utils.h"

char *String_Utils_concat(char *string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, NULL);
    VALIDATE_PTR(string_two, NULL);
    char *temp = malloc(strlen(string_one) + strlen(string_two) + 1);
    memset(temp, 0, strlen(string_one) + strlen(string_two) + 1);
    strcat(temp, string_one);
    strcat(temp, string_two);
    // Based on parameters below, will modify the original screen, or just return the new string.
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string_one, temp, NONE);
        free(temp);
        return string_one;
    } else return temp; // Not modifying the original string is the default, even if there was no valid parameter.
}

void String_Utils_update(String_Utils *self) { // IMPLEMENT!
    //
}

int String_Utils_compare(char *string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, -1);
    VALIDATE_PTR(string_two, -1);
    if (SELECTED(parameter, IGNORE_CASE)) return strcmp(String_Utils_copy(string_one, LOWERCASE), String_Utils_copy(string_two, LOWERCASE));
    else return strcmp(string_one, string_two);
}

char String_Utils_char_at(char *string, unsigned int index) {
    VALIDATE_PTR(string, '\0');
    return string[index > strlen(string) ? strlen(string) - 1 : index];
}

int String_Utils_contains(char *string, char *search, int parameter) { // FIX!
    VALIDATE_PTR(string, 0);
    VALIDATE_PTR(search, 0);
    if (SELECTED(parameter, IGNORE_CASE)) return strstr(String_Utils_copy(string, LOWERCASE), String_Utils_copy(search, LOWERCASE)) == NULL ? 0 : 1;
    else return strstr(string, search) == NULL ? 0 : 1;
}

char *String_Utils_to_lowercase(char *string, int parameter) {
    VALIDATE_PTR(string, NULL);
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for (i; i <= strlen(string); i++) {
        temp[i] = tolower(string[i]);
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

char *String_Utils_to_uppercase(char *string, int parameter) {
    VALIDATE_PTR(string, NULL);
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for (i; i <= strlen(string); i++) {
        temp[i] = toupper(string[i]);
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

unsigned int *String_Utils_get_bytes(char *string) {
    VALIDATE_PTR(string, NULL);
    unsigned int *bytes = malloc(sizeof (unsigned int) * strlen(string));
    int i = 0;
    for (i; i < strlen(string); i++) bytes[i] = (unsigned int) ((unsigned char) string[i]);
    return bytes;
}

int String_Utils_equals(char *string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, 0);
    VALIDATE_PTR(string_two, 0);
    return String_Utils_compare(string_one, string_two, parameter) == 0 ? 1 : 0;
}

char *String_Utils_copy(char *string, int parameter) {
    VALIDATE_PTR(string, NULL);
    char *temp = malloc(strlen(string) + 1);
    strcpy(temp, string);
    if(SELECTED(parameter, LOWERCASE)) temp = String_Utils_to_lowercase(temp, NONE);
    if(SELECTED(parameter, UPPERCASE)) temp = String_Utils_to_uppercase(temp, NONE);
    if(SELECTED(parameter, REVERSE)) temp = String_Utils_reverse(temp, NONE);
    return temp;
}

char *String_Utils_from(char *string, unsigned int index, int parameter) {
    VALIDATE_PTR(string, NULL);
    int i = index > strlen(string) ? strlen(string)-1 : index ;
    int j = i;
    char *temp = malloc((strlen(string) + 1) - i);
    
    for (i; i <= strlen(string); i++) {
        temp[i - j] = string[i];
    }
    if (parameter == MODIFY) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

int String_Utils_length(char *string) {
    VALIDATE_PTR(string, 0);
    return strlen(string);
}

char *String_Utils_from_token(char *string, char *delimiter, int parameter) {
    VALIDATE_PTR(string, NULL);
    VALIDATE_PTR(delimiter, NULL);
    char *temp = strstr(string, delimiter);
    if(temp == NULL) return NULL;
    if(SELECTED(parameter, LAST)){
        while(temp != NULL) temp = strstr(string, delimiter);
    }
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    }
    return temp;
}

char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter) {
    VALIDATE_PTR(string, NULL);
    VALIDATE_PTR(delimiter, NULL);
    VALIDATE_PTR(size, NULL);
    char **string_array = malloc(sizeof (char *)); // Inspect here if crash due to bad allocations!
    char *temp;
    char *temp_string = String_Utils_copy(string, NONE);
    temp = strtok(temp_string, delimiter);
    if (temp == NULL) return NULL;
    unsigned int index = 0;
    while (temp != NULL) {
        string_array[index] = malloc(strlen(temp) + 1);
        strcpy(string_array[index], temp);
        index++;
        string_array = realloc(string_array, (sizeof (char *) * (index + 1))); 
        temp = strtok(NULL, delimiter);
    }
    free(temp);
    free(temp_string);
    *size = index;
    return string_array;
}

char *String_Utils_set(char **string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, NULL);
    VALIDATE_PTR(*string_one, NULL);
    VALIDATE_PTR(string_two, NULL);
    char *temp_string;
    temp_string = String_Utils_copy(string_two, parameter);
    *string_one = realloc(*string_one, strlen(temp_string) + 1);
    memset(*string_one, '\0', strlen(temp_string)+1);
    strcpy(*string_one, temp_string);
    free(temp_string);
    return *string_one;
}

char *String_Utils_concat_all(int parameter, unsigned int amount, char *string, ...) {
    VALIDATE_PTR(string, NULL);
    va_list args;
    int i = 0;
    //size_t size = strlen(string) + 1;
    va_start(args, string);
    char *final_string = malloc(strlen(string));
    final_string = String_Utils_concat(final_string, string, NONE);
    char *temp;
    for(i; i < amount; i++){
        temp = va_arg(args, char *);
        final_string = String_Utils_concat(final_string, temp, NONE);
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, final_string, NONE);
        free(final_string);
        return string;
    } else return final_string;
}

char *String_Utils_reverse(char *string, int parameter) {
    VALIDATE_PTR(string, NULL);
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for (i; i <= strlen(string); i++) {
        temp[i] = string[strlen(string) - 1 - i];
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

char *String_Utils_replace(char *string, char old_char, char new_char, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    if(SELECTED(parameter, IGNORE_CASE)){
        for(i; i <= strlen(string); i++){
            if(tolower(string[i]) == tolower(old_char)) temp[i] = new_char;
            else temp[i] = string[i];
        }
    }
    else {
        for(i; i <= strlen(string); i++){
            if(string[i] == old_char) temp[i] = new_char;
            else temp[i] = string[i];
        }
    }
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

char *String_Utils_join(char **array_of_strings, char *delimiter, size_t *size, int parameter){
    VALIDATE_PTR(array_of_strings, NULL);
    VALIDATE_PTR(size, NULL);
    VALIDATE_PTR(delimiter, NULL);
    char *temp = NULL;
    int i = 0;
    for(i; i < *size; i++){
        if(temp == NULL) temp = String_Utils_copy(array_of_strings[i], NONE);
        else temp = String_Utils_concat_all(NONE, 2, temp, delimiter, array_of_strings[i]);
    }
    return temp;
}

int String_Utils_starts_with(char *string, char *find, int parameter){
    VALIDATE_PTR(string, 0); VALIDATE_PTR(find, 0); 
    int i = 0;
    for(i; i < strlen(find); i++) if(SELECTED(parameter, IGNORE_CASE)){
        if(tolower(string[i]) != tolower(find[i])) return 0;
    } else if(string[i] != find[i]) return 0;
    return 1;
}

int String_Utils_ends_with(char *string, char *find, int parameter){
    VALIDATE_PTR(string, 0); VALIDATE_PTR(find, 0);
    int i = strlen(string) - strlen(find);
    for(i; i < strlen(string); i++){
        if(SELECTED(parameter, IGNORE_CASE)){
            if(tolower(string[i]) != tolower(find[i - (strlen(string) - strlen(find))])) return 0; 
        } else if(string[i] != find[i - (strlen(string) - strlen(find))]) return 0;
    }
    return 1;
}

void String_Utils_free_array(char **array, size_t size){
    VALIDATE_PTR_VOID(array);
    int i = 0;
    for(i; i < size; i++){
        free(array[i]);
    }
    //free(array);
    //array = NULL;
}

char *String_Utils_capitalize(char *string, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp = String_Utils_copy(string, NONE);
    temp[0] = toupper(temp[0]);
    if(SELECTED(parameter, MODIFY)) String_Utils_set(&string, temp, NONE);
    return temp;
}

char *String_Utils_trim(char *string, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp;
    int i = 0;
    int j = strlen(string) -1;
    for(i; i < strlen(string); i++){
        if(!isspace(string[i])) break;
    }
    for(j; j > i ; j--){
        if(!isspace(string[j])) break;
    }
    temp = String_Utils_substring(string, i, j, NONE);
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    }
    return temp;
}

char *String_Utils_substring(char *string, unsigned int begin, unsigned int end, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp = malloc((end - begin) + 1);
    memcpy(temp, string + begin, end < begin ? strlen(string) - begin : end - begin);
    temp = String_Utils_copy(temp, parameter); // Forwards parameter to copy
    return temp;
}

int String_Utils_index_of(char *string, char *token, int parameter){
    VALIDATE_PTR(string, -1);
    VALIDATE_PTR(token, -1);
    char *temp;
    if(SELECTED(parameter, IGNORE_CASE)) temp = strstr(String_Utils_to_lowercase(string, NONE), String_Utils_to_lowercase(token, NONE));
    else temp = strstr(string, token);
    if(temp == NULL) { DEBUG_PRINT("NULL returned, returning negative number!\n"); return -1; }
    return strlen(string) - strlen(temp);
}

int String_Utils_count(char *string, char *substring, int parameter){
    VALIDATE_PTR(string, 0);
    VALIDATE_PTR(substring, 0);
    int count = 0;
    char *temp; 
    if(SELECTED(parameter, IGNORE_CASE)) temp = String_Utils_copy(string, LOWERCASE);
    else temp = String_Utils_copy(string, NONE);
    while(temp = strstr(temp, substring)){ // temp gets set to the next occurrence of the found substring 
        count++;
        //printf("Count is %d\nCurrent string being compared: %s\nString length = %d\nSubstring length: %d\n", count, string, strlen(string), strlen(substring));
        if(strlen(temp) < strlen(substring)) temp = NULL;
        else temp += strlen(substring);
        //printf("New String: %s\nNew String Length: %d\n", temp, strlen(temp));
    }
    return count;
}

char *String_Utils_between(char *string, char *start, char *end, int parameter){
    char *temp; 
    if(SELECTED(parameter, IGNORE_CASE)) temp = strstr(String_Utils_copy(string, LOWERCASE), String_Utils_copy(start, LOWERCASE)) + strlen(start);
    else temp = strstr(string, start) + strlen(start);
    size_t size = strlen(temp) - (strlen(strstr(string, end)));
    temp = String_Utils_substring(temp, 0, size, NONE);
    return temp;
}

String_Utils *String_Utils_create(void) {
    String_Utils *string = malloc(sizeof (String_Utils));
    string->get_bytes = INIT(get_bytes);
    string->length = INIT(length);
    string->index_of = INIT(index_of);
    string->free_array = INIT(free_array);
    string->replace = INIT(replace);
    string->join = INIT(join);
    string->reverse = INIT(reverse);
    string->set = INIT(set);
    string->split = INIT(split);
    string->starts_with = INIT(starts_with);
    string->substring = INIT(substring);
    string->to_uppercase = INIT(to_uppercase);
    string->to_lowercase = INIT(to_lowercase);
    string->trim = INIT(trim);
    string->equals = INIT(equals);
    string->capitalize =  INIT(capitalize);
    string->copy = INIT(copy);
    string->count = INIT(count);
    string->compare = INIT(compare);
    string->concat = INIT(concat);
    string->concat_all = INIT(concat_all);
    string->contains = INIT(contains);
    string->ends_with = INIT(ends_with);
    string->from = INIT(from);
    string->from_token = INIT(from_token);
    string->between = INIT(between);
    printf("String_Utils initialized!\n");
    return string;
}