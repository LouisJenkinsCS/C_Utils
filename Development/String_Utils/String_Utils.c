#include "String_Utils.h"

char *String_Utils_concat(char *string_one, char *string_two, int parameter) {
    char *temp = malloc(strlen(string_one) + strlen(string_two) + 1);
    memset(temp, 0, strlen(string_one) + strlen(string_two) + 1);
    strcat(temp, string_one);
    strcat(temp, string_two);
    // Based on parameters below, will modify the original screen, or just return the new string.
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(string_one, temp, NONE);
        free(temp);
        return string_one;
    } else return temp; // Not modifying the original string is the default, even if there was no valid parameter.
}

void String_Utils_update(String_Utils *self) { // IMPLEMENT!
    //
}

int String_Utils_compare(char *string_one, char *string_two, int parameter) {
    if (SELECTED(parameter, IGNORE_CASE)) return strcmp(String_Utils_to_lowercase(string_one, NO_MODIFY), String_Utils_to_lowercase(string_two, NO_MODIFY));
    else return strcmp(string_one, string_two);
}

char String_Utils_char_at(char *string, unsigned int index) {
    return string[index > strlen(string) ? strlen(string) - 1 : index];
}

int String_Utils_contains(char *string, char *search, int parameter) { // FIX!
    if (SELECTED(parameter, IGNORE_CASE)) return strstr(String_Utils_to_lowercase(string, NO_MODIFY), String_Utils_to_lowercase(search, NO_MODIFY)) == NULL ? 0 : 1;
    else return strstr(string, search) == NULL ? 0 : 1;
}

char *String_Utils_to_lowercase(char *string, int parameter) {
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for (i; i <= strlen(string); i++) {
        temp[i] = tolower(string[i]);
    }
    if (SELECTED(parameter, MODIFY)) {
        strcpy(string, temp);
        free(temp);
        return string;
    } else return temp;
}

char *String_Utils_to_uppercase(char *string, int parameter) {
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for (i; i <= strlen(string); i++) {
        temp[i] = toupper(string[i]);
    }
    if (SELECTED(parameter, MODIFY)) {
        strcpy(string, temp);
        free(temp);
        return string;
    } else return temp;
}

unsigned int *String_Utils_get_bytes(char *string) {
    unsigned int *bytes = malloc(sizeof (unsigned int) * strlen(string));
    int i = 0;
    for (i; i < strlen(string); i++) bytes[i] = (unsigned int) ((unsigned char) string[i]);
    return bytes;
}

int String_Utils_equals(char *string_one, char *string_two, int parameter) {
    return String_Utils_compare(string_one, string_two, parameter) == 0 ? 1 : 0;
}

char *String_Utils_copy(char *string) {
    char *temp = malloc(strlen(string) + 1);
    strcpy(temp, string);
    return temp;
}

char *String_Utils_from(char *string, unsigned int index, int parameter) {
    char *temp = malloc((strlen(string) + 1) - index);
    int i = index;
    for (i; i <= strlen(string); i++) {
        temp[i - index] = string[i];
    }
    if (parameter == MODIFY) {
        String_Utils_set(string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

int String_Utils_length(char *string) {
    return strlen(string);
}

char *String_Utils_from_token(char *string, char *delimiter, int parameter) {
    char **tokens;
    char *temp;
    size_t *size = malloc(sizeof(size_t));
    if (SELECTED(parameter, MODIFY)) tokens = String_Utils_split(string, delimiter, size, MODIFY);
    else tokens = String_Utils_split(string, delimiter, size, NO_MODIFY);
    if(tokens == NULL) return NULL;
    if (SELECTED(parameter, FIRST)) {
        int i = 1;
        for(i; i < *size; i++){
            if(temp == NULL) temp = String_Utils_copy(tokens[i]);
            else {
                temp = String_Utils_concat(temp, delimiter, NONE); // Adds the delimiter back
                temp = String_Utils_concat(temp, tokens[i], NONE);
            }
        }
    }
    if(SELECTED(parameter, LAST)){
        temp = String_Utils_copy(tokens[*size-1]);
    }
    // Free the array of strings
    int i = 0;
    for(i; i < *size; i++) free(tokens[i]);
    free(tokens);
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(string, temp, NONE);
        free(temp);
        return string;
    }
    return temp;
}

char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter) {
    char **string_array = malloc(sizeof (char *)); // Inspect here if crash due to bad allocations!
    char *temp;
    char *temp_string = String_Utils_copy(string);
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

char *String_Utils_set(char *string_one, char *string_two, int parameter) {
    char *temp_string;
    if (SELECTED(parameter, REVERSE)) {
        temp_string = String_Utils_reverse(string_two, NO_MODIFY);
    } else temp_string = String_Utils_copy(string_two);
    string_one = realloc(string_one, strlen(temp_string) + 1);
    strcpy(string_one, temp_string);
    free(temp_string);
    return string_one;
}

char *String_Utils_concat_all(int parameter, unsigned int amount, char *string, ...) {
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
    if (parameter == MODIFY) {
        String_Utils_set(string, final_string, NONE);
        free(final_string);
        return string;
    } else return final_string;
}

char *String_Utils_reverse(char *string, int parameter) {
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for (i; i < strlen(string); i++) {
        temp[i] = string[strlen(string) - 1 - i];
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

char *String_Utils_replace(char *string, char old_char, char new_char, int parameter){
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
        String_Utils_set(string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

char *String_Utils_join(char **array_of_strings, size_t *size, int parameter){
    char *temp;
    int i = 0;
    for(i; i < *size; i++){
        if(temp == NULL) temp = String_Utils_copy(array_of_strings[i]);
        else temp = String_Utils_concat(temp, array_of_strings[i], NONE);
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
        } else if(string[i] != find[i]) return 0;
    }
    return 1;
}

void String_Utils_free_array(char **array, size_t size){
    VALIDATE_PTR(array, NULL);
    int i = 0;
    for(i; i < size; i++){
        free(array[i]);
    }
    free(array);
    array = NULL;
}

char *String_Utils_capitalize(char *string, int parameter){
    char *temp = String_Utils_copy(string);
    temp[0] = toupper(temp[0]);
    if(SELECTED(parameter, MODIFY)) String_Utils_set(string, temp, NONE);
    return temp;
}

char *String_Utils_trim(char *string, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp;
    int i = 0;
    int j = strlen(string);
    for(i; i < strlen(string); i++){
        if(!isspace(string[i])) break;
    }
    for(j; j > i ; j--){
        if(!isspace(string[j])) break;
    }
    temp = String_Utils_substring(string, i, j, NONE);
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(string, temp, NONE);
        free(temp);
        return string;
    }
    return temp;
}

char *String_Utils_substring(char *string, unsigned int begin, unsigned int end, int parameter){
    char *temp = malloc(end - begin);
    memcpy(temp, string + begin, end - begin);
    if(SELECTED(parameter, REVERSE)) temp = String_Utils_reverse(temp, NONE);
    return temp;
}

String_Utils *String_Utils_create(void) {
    String_Utils *string = malloc(sizeof (String_Utils));
    string->concat = String_Utils_concat;
    string->compare = String_Utils_compare;
    string->update = String_Utils_update;
    string->char_at = String_Utils_char_at;
    string->contains = String_Utils_contains;
    string->get_bytes = String_Utils_get_bytes;
    string->equals = String_Utils_equals;
    return string;
}