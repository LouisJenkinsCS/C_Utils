#include "String_Utils.h"
/*
 * To force myself to reflect on my errors, I am going to document every
 * allocation and deallocation of memory.
 * 
 * Notes to self: Might want to change MODIFY sections of functions to
 * pass the parameter directly to set. 
 */


/*
 * Should not produce any memory leaks, anything allocated is deallocated.
 */
char *String_Utils_concat(char *string_one, char *string_two, int parameter) {
    char *temp = NULL;
    VALIDATE_PTR(string_one, NULL);
    VALIDATE_PTR(string_two, NULL);
    // Allocate temporary string to size of two strings, + 1 for null terminator
    temp = malloc(strlen(string_one) + strlen(string_two) + 1);
    // Zeroes the struct to clean it.
    memset(temp, 0, strlen(string_one) + strlen(string_two) + 1);
    // Copies the first string to temp
    strcpy(temp, string_one);
    // Concatenates string_two to temp
    strcat(temp, string_two);
    // Based on parameters below, will modify the original screen, or just return the new string.
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string_one, temp, NONE);
        // Free temp as no longer needed.
        free(temp);
        return string_one;
    } else return temp; // Not modifying the original string is the default, even if there was no valid parameter.
}

void String_Utils_update(String_Utils *self) { // IMPLEMENT!
    //
}

/*
 * This function should now no longer produce a memory leak, although it will allocate
 * two temporary strings on the heap rather than the stack before freeing them.
 * 
 * Note to Self: Optimize this to allocate on stack instead of heap.
 */
int String_Utils_compare(char *string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, -1);
    VALIDATE_PTR(string_two, -1);
    // Allocate two temporary strings
    char *temp_string_one = String_Utils_copy(string_one, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp_string_two = String_Utils_copy(string_two, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    // Store result of string comparison
    int result =  strcmp(temp_string_one, temp_string_two);
    // Free both temporary strings
    free(temp_string_one);
    free(temp_string_two);
    return result;
}

/*
 * Does not allocate any memory.
 */
char String_Utils_char_at(char *string, unsigned int index) {
    VALIDATE_PTR(string, '\0');
    return string[index > strlen(string) - 1 ? strlen(string) - 1 : index];
}

/*
 * Like Compare, now allocates strings to hold the copies, then deallocates them. 
 */
int String_Utils_contains(char *string, char *search, int parameter) { // FIX!
    VALIDATE_PTR(string, 0);
    VALIDATE_PTR(search, 0);
    // Allocates temporary variables to hold copy of the passed strings.
    char *temp_string = String_Utils_copy(string, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp_search = String_Utils_copy(search, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    int result = strstr(temp_string, temp_search) == NULL ? 0 : 1;
    // Free both temporary strings
    free(temp_string);
    free(temp_search);
    return result;
}

/*
 * Properly allocates and deallocates memory.
 */
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

/*
 * Properly allocates and deallocates memory.
 */
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

/*
 * Properly allocates and deallocates memory.
 */
unsigned int *String_Utils_get_bytes(char *string) {
    VALIDATE_PTR(string, NULL);
    unsigned int *bytes = malloc(sizeof (unsigned int) * strlen(string));
    int i = 0;
    for (i; i < strlen(string); i++) bytes[i] = (unsigned int) ((unsigned char) string[i]);
    return bytes;
}

/*
 * Properly allocates and deallocates memory.
 */
int String_Utils_equals(char *string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, 0);
    VALIDATE_PTR(string_two, 0);
    return String_Utils_compare(string_one, string_two, parameter) == 0 ? 1 : 0;
}

/*
 * Fixed to not arbitrarily create copies of temp, losing the old value to be
 * another memory leak. This way, based on the parameter, temp will be modified
 * returning the pointer to temp, assigning it to itself. So where it points
 * never actually changes, just the value of where it's pointing to.
 */
char *String_Utils_copy(char *string, int parameter) {
    VALIDATE_PTR(string, NULL);
    // Allocates a temporary string to hold copy of string
    char *temp = malloc(strlen(string) + 1);
    strcpy(temp, string);
    // The below will modify temp the appropriate parameter.
    if(SELECTED(parameter, LOWERCASE)) temp = String_Utils_to_lowercase(temp, MODIFY);
    if(SELECTED(parameter, UPPERCASE)) temp = String_Utils_to_uppercase(temp, MODIFY);
    if(SELECTED(parameter, REVERSE)) temp = String_Utils_reverse(temp, MODIFY);
    return temp;
}

/*
 * Properly allocates and deallocates memory.
 */
char *String_Utils_from(char *string, unsigned int index, int parameter) {
    VALIDATE_PTR(string, NULL);
    int i = index > strlen(string)-1 ? strlen(string)-1 : index ;
    int j = i;
    // Allocate temporary variable.
    char *temp = malloc((strlen(string) + 1) - i);
    
    for (i; i <= strlen(string); i++) {
        temp[i - j] = string[i];
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

/*
 * Does not allocate any memory.
 */
int String_Utils_length(char *string) {
    VALIDATE_PTR(string, 0);
    return strlen(string);
}

/*
 * Fixed so it should not produce a memory leak, although it feels like it's more
 * complicated than it should be. Hence, I'll leave a note here.
 * Note: This function, while should no longer produce a memory leak, should be
 * optimized/simplified in the future!
 * 
 * Note: IGNORE_CASE is broken, please fix!
 */
char *String_Utils_from_token(char *string, char *substring, int parameter) {
    VALIDATE_PTR(string, NULL);
    VALIDATE_PTR(substring, NULL);
    // Allocates a temporary copy of substring depending on parameter passed.
    char *temp_string = String_Utils_copy(string, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp_substring = String_Utils_copy(substring, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp = strstr(temp_string, temp_substring);
    char *old_temp = NULL;
    if(temp == NULL) return NULL;
    if(SELECTED(parameter, LAST)){
        while(temp != NULL) {
            temp = strstr(temp_string, temp_substring); 
            if(temp != NULL) {
                // Frees the previous copy of old_temp.
                if(old_temp != NULL) free(old_temp); 
                // Assigns old_temp to temp
                old_temp = String_Utils_copy(temp, NONE); 
            }
        }
        temp = String_Utils_copy(old_temp, NONE);
    }
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, old_temp, NONE);
        //free everything
        free(temp_substring);
        free(temp_string);
        free(temp);
        free(old_temp);
        return string;
    }
    // free everything but temp
    free(temp_substring);
    free(old_temp);
    return temp;
}

/*
 * Correctly allocates and deallocates memory.
 */
char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter) {
    VALIDATE_PTR(string, NULL);
    VALIDATE_PTR(delimiter, NULL);
    VALIDATE_PTR(size, NULL);
    // Allocates an array of strings, initial size of only one string.
    char **string_array = malloc(sizeof (char *));
    char *temp = NULL;
    // Allocated a temporary string so original string is not modified from strtok.
    char *temp_string = String_Utils_copy(string, NONE);
    temp = strtok(temp_string, delimiter);
    if (temp == NULL) return NULL;
    unsigned int index = 0;
    while (temp != NULL) {
        // The index in the array is allocated
        string_array[index] = malloc(strlen(temp) + 1);
        strcpy(string_array[index], temp);
        index++;
        // Reallocates the array so it can hold another array.
        string_array = realloc(string_array, (sizeof (char *) * (index + 1))); 
        temp = strtok(NULL, delimiter);
    }
    free(temp);
    free(temp_string);
    *size = index;
    return string_array;
}

/*
 * Should appropriately (and efficiently) set one string to another.
 * A copy of string_two is allocated and then freed because the 
 * parameter is handled in copy.
 */
char *String_Utils_set(char **string_one, char *string_two, int parameter) {
    VALIDATE_PTR(string_one, NULL);
    VALIDATE_PTR(*string_one, NULL);
    VALIDATE_PTR(string_two, NULL);
    printf("Value of *string_one = %s\nMemory Address of *String_one = %p\n String_two = %s\n", *string_one, *string_one, string_two);
    char *temp_string_two = String_Utils_copy(string_two, parameter);
    // Reallocates the size of the string.
    free(*string_one);
    *string_one = malloc(strlen(temp_string_two) + 1);
    // Clears out the string
    memset(*string_one, 0 , strlen(temp_string_two)+1);
    // Copy string_two into string_one
    strcpy(*string_one, temp_string_two);
    free(temp_string_two);
    printf("Final result of *String_one = %s\nFinal Memory Address of *String_one = %p\n", *string_one, *string_one);
    return *string_one;
}

/*
 * Fixed to note leak memory all over the place. Changed concat to use MODIFY parameter
 * and added va_end(args) to prevent leakage.
 */
char *String_Utils_concat_all(int parameter, unsigned int amount, char *string, ...) {
    VALIDATE_PTR(string, NULL);
    va_list args;
    int i = 0;
    va_start(args, string);
    char *final_string = String_Utils_copy(string, NONE);
    char *temp = NULL;
    for(i; i < amount; i++){
        temp = va_arg(args, char *);
        final_string = String_Utils_concat(final_string, temp, MODIFY);
    }
    va_end(args);
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, final_string, NONE);
        free(final_string);
        return string;
    } else return final_string;
}

/*
 * For loop should correctly reverse the string without underflowing like before.
 */
char *String_Utils_reverse(char *string, int parameter) {
    VALIDATE_PTR(string, NULL);
    char *temp = malloc(strlen(string) + 1);
    int i, j;
    for (i = 0, j = strlen(string) - 1; i < strlen(string); i++, j--) {
        temp[i] = string[j];
    }
    temp[strlen(string)] = '\0';
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

/*
 * Correctly allocates and deallocates memory.
 */
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

/*
 * Join now passes the MODIFY parameter so as not to create an arbitrary amount
 * of strings.
 */
char *String_Utils_join(char **array_of_strings, char *delimiter, size_t *size, int parameter){
    VALIDATE_PTR(array_of_strings, NULL);
    VALIDATE_PTR(size, NULL);
    VALIDATE_PTR(delimiter, NULL);
    char *temp = NULL;
    int i = 0;
    for(i; i < *size; i++){
        if(temp == NULL) temp = String_Utils_copy(array_of_strings[i], NONE);
        else temp = String_Utils_concat_all(MODIFY, 2, temp, delimiter, array_of_strings[i]);
    }
    return temp;
}

/*
 * Does not allocate any memory.
 */
int String_Utils_starts_with(char *string, char *find, int parameter){
    VALIDATE_PTR(string, 0); VALIDATE_PTR(find, 0); 
    int i = 0;
    for(i; i < strlen(find); i++) if(SELECTED(parameter, IGNORE_CASE)){
        if(tolower(string[i]) != tolower(find[i])) return 0;
    } else if(string[i] != find[i]) return 0;
    return 1;
}

/*
 * Does not allocate any memory.
 * Note: Modified for loop to keep track of variables i and j instead of just j.
 */
int String_Utils_ends_with(char *string, char *find, int parameter){
    VALIDATE_PTR(string, 0); VALIDATE_PTR(find, 0);
    int i, j;
    for(i = strlen(string) - strlen(find), j = 0; i < strlen(string); i++, j++){
        if(SELECTED(parameter, IGNORE_CASE)){
            if(tolower(string[i]) != tolower(find[j])) return 0; 
        } else if(string[i] != find[j]) return 0;
    }
    return 1;
}

/*
 * Note: Might want to make it a char ***array instead to correctly destroy it.
 */
void String_Utils_free_array(char **array, size_t size){
    VALIDATE_PTR_VOID(array);
    int i = 0;
    for(i; i < size; i++){
        free(array[i]);
    }
    //free(array);
    //array = NULL;
}

/*
 * Should work as intended without memory leaks. 
 */
char *String_Utils_capitalize(char *string, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp = String_Utils_copy(string, NONE);
    temp[0] = toupper(temp[0]);
    if(SELECTED(parameter, MODIFY)) { String_Utils_set(&string, temp, NONE); free(temp); return string; }
    return temp;
}

/*
 * Correctly allocates and deallocates memory.
 */
char *String_Utils_trim(char *string, int parameter){
    VALIDATE_PTR(string, NULL);
    char *temp = NULL;
    int i = 0;
    int j = strlen(string)-1;
    for(i; i < strlen(string); i++){
        if(!isspace(string[i])) break;
    }
    for(j; j > i ; --j){
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

/*
 * Should correctly allocate and deallocate memory without causing leaks.
 */
char *String_Utils_substring(char *string, unsigned int begin, unsigned int end, int parameter){
    VALIDATE_PTR(string, NULL);
    // If begin is greater than the end, set new_begin to 0, else equal to begin
    unsigned int new_begin = begin > end ? 0 : begin; // Bounds checking for begin
    // If end is out of bounds, set new_end to the length of the string - 1, else equal to end
    unsigned int new_end = end >= strlen(string) ? strlen(string) - 1 : end;
    size_t size = new_end - new_begin; // The size of the substring will be end - begin
    char *temp = malloc(size + 1); // Allocate a temporary variable to hold substring.
    char *new_temp = NULL; // Used to hold the modified copy of temp based on parameters passed.
    memset(temp, 0, size + 1); // Zeroes the struct to clean it.
    memcpy(temp, string + begin, size + 1); // Copy into temp (destination), the contents of string[begin] size bytes.
    temp[size + 1] = '\0'; // Append a null terminator to the temporary string.
    new_temp = String_Utils_copy(temp, parameter); // Forwards parameter to copy
    free(temp); // Free old temp variable.
    if(SELECTED(parameter, MODIFY)) { // Modifies the original string is parameter is passed.
        String_Utils_set(&string, new_temp, NONE);
        free(new_temp); 
        return string; 
    }
    return new_temp;
}

/*
 * Overly complicated this function. Most likely it's extremely inefficient and more
 * complicated than it needs to be. Marking a note here for later.
 * Note: Please revise this function? It's awful D:
 */
int String_Utils_index_of(char *string, char *substring, int parameter){
    VALIDATE_PTR(string, -1);
    VALIDATE_PTR(substring, -1);
    char *temp = NULL;
    char *temp_string = String_Utils_copy(string, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp_substring = String_Utils_copy(substring, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *old_temp = NULL;
    temp = strstr(temp_string, temp_substring);
    if(temp == NULL) { 
        DEBUG_PRINT("NULL returned, returning negative number!\n"); 
        // Free memory
        free(temp_string);
        free(temp_substring);
        return -1; 
    }
    if(SELECTED(parameter, LAST)){
        while(temp != NULL) {
            temp = strstr(temp_string, temp_substring);
            if(temp != NULL){
                // Free old_temp before assigning it to copy of temp.
                if(old_temp == NULL) free(old_temp);
                old_temp = String_Utils_copy(temp, NONE); 
            }
        }
        temp = String_Utils_copy(old_temp, NONE);
    }
    free(old_temp); 
    int result = strlen(string) - strlen(temp);
    free(temp_string);
    free(temp_substring);
    //free(temp);
    return result;
}

/*
 * Should work correctly without memory leakage.
 */
int String_Utils_count(char *string, char *substring, int parameter){
    VALIDATE_PTR(string, 0);
    VALIDATE_PTR(substring, 0);
    int count = 0;
    char *temp = NULL; 
    char *temp_ptr = NULL;
    temp = String_Utils_copy(string, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    temp_ptr = temp; // For when temp gets set to NULL, will be able to keep track of it to be freed later.
    while(temp = strstr(temp, substring)){ // temp gets set to the next occurrence of the found substring 
        count++;
        //printf("Count is %d\nCurrent string being compared: %s\nString length = %d\nSubstring length: %d\n", count, string, strlen(string), strlen(substring));
        if(strlen(temp) < strlen(substring)) temp = NULL;
        else temp += strlen(substring);
        //printf("New String: %s\nNew String Length: %d\n", temp, strlen(temp));
    }
    free(temp_ptr);
    return count;
}

/*
 * Uggghhh, this function bad. Even if it works, it forces so many temporary
 * variables that it should be a crime to do so. I had to add aforementioned 
 * temporary variables to replace all of the copies of the string I made without
 * deallocating them.
 */
char *String_Utils_between(char *string, char *start, char *end, int parameter){
    VALIDATE_PTR(string, NULL);
    VALIDATE_PTR(start, NULL);
    VALIDATE_PTR(end, NULL);
    char *temp = NULL; 
    char *new_temp = NULL;
    char *temp_string = String_Utils_copy(string, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp_start = String_Utils_copy(start, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    char *temp_end = String_Utils_copy(end, SELECTED(parameter, IGNORE_CASE) ? LOWERCASE : NONE);
    temp = strstr(temp_string, temp_start) + strlen(temp_start);
    if(temp == NULL) return NULL;
    size_t size = strlen(temp) - (strlen(strstr(temp_string, temp_end)));
    new_temp = String_Utils_substring(temp, 0, size, NONE);
    free(temp);
    free(temp_string);
    free(temp_start);
    free(temp_end);
    return new_temp;
}

String_Utils *String_Utils_create(void) {
    String_Utils *string = malloc(sizeof (String_Utils));
    if(string == NULL){ DEBUG_PRINT("Unable to create String_Utils; Unable to allocate memory!\n"); return NULL; }
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
    string->char_at = INIT(char_at);
    DEBUG_PRINT("String_Utils initialized!\n");
    return string;
}