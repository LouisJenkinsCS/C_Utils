#include "String_Utils.h"
#include "String_Utils_Garbage_Collector.h"
/*
 * I decided to use String_Utils to test my new garbage collector. Not all functions use it,
 * only ones that allocated 2 or more temporary variables. This is more of a trial run, there
 * is no heap currently implemented in the virtual machine, only a basic stack that pushes new values
 * on the stack, pops them off of the stack, and destroying the virtual machine will activate the
 * garbage collector's mark and sweep algorithm to free everything. 
 *
 * Also I decided to not care about portability much, I'm going to program almost exclusively in C
 * from now on, so there's no point. The glib c is better than anything I can make, so may as well
 * use theirs in my own program.
 */


/*
 * Using the wonderful asprintf function that returns the string of a formatted function.
 * I'm certain that the performance of this function is 100x better than strcpy into
 * a cleaned allocated temp, then strcat on top of that.
 */
char *String_Utils_concat(char *string_one, const char *string_two, int parameter) {
    assert(string_one);
    assert(string_two);
    // Allocate temporary string to size of two strings, + 1 for null terminator
    char *temp = malloc(strlen(string_one) + strlen(string_two) + 1);
    // GLIB C to the rescue!
    asprintf(&temp, "%s%s", string_one, string_two);
    // Based on parameters below, will modify the original screen, or just return the new string.
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string_one, temp);
        // Free temp as no longer needed.
        free(temp);
        return string_one;
    } else return temp; // Not modifying the original string is the default, even if there was no valid parameter.
}


/*
 * Uses my new basic garbage collector. Honestly, it's not really anything special, and it could be done easily 
 * with basic frees over pushing and popping them from the stack, but this is a trial run. Also decided to
 * get rid of my copy function for strdup, which does it a lot better than I ever could anyway. It didn't
 * serve any different purpose in the end.
 */
int String_Utils_compare(const char *string_one, const char *string_two, int parameter) {
    assert(string_one);
    assert(string_two);
    int result = 0;
    // Allocate two temporary strings
    if(SELECTED(parameter, IGNORE_CASE)){
        char *temp_string_one = strdup(string_one);
        char *temp_string_two = strdup(string_two);
        put_strings(vm, 2, temp_string_one, temp_string_two); // Puts both elements on stack.
        result = strcmp(temp_string_one, temp_string_two);
        pop_vn(vm, 2); // Pops both elements off of stack
    } else result = strcmp(string_one, string_two);
    return result;
}

/*
 * Basic index checks.
 */
char String_Utils_char_at(const char *string, unsigned int index) {
    assert(string);
    return string[index > strlen(string) - 1 ? strlen(string) - 1 : index];
}

/*
 * Like Compare, uses GC. Not much new.
 */
int String_Utils_contains(const char *string, const char *search, int parameter) { 
    assert(string);
    assert(search);
    int result = 0;
    // Allocates temporary variables to hold copy of the passed strings.
    if(SELECTED(parameter, IGNORE_CASE)){
        char *temp_string_one = strdup(string);
        char *temp_string_two = strdup(search);
        put_strings(vm, 2, temp_string_one, temp_string_two); // Puts both elements on stack.
        result = strstr(temp_string, temp_search) == NULL ? 0 : 1;
        pop_vn(vm, 2); // Pops both elements off of stack
    } else result = strstr(string, search) == NULL ? 0 : 1;
    return result;
}

/*
 * Properly allocates and deallocates memory.
 */
char *String_Utils_to_lowercase(char *string, int parameter) {
    assert(string);
    char *temp = malloc(strlen(string) + 1); // Note: This does not need to be added to the stack.
    int i = 0;
    for (i; i <= strlen(string); i++) {
        temp[i] = tolower(string[i]);
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp; // If the heap existed, temp would be added to it before return.
}

/*
 * Properly allocates and deallocates memory.
 */
char *String_Utils_to_uppercase(char *string, int parameter) {
    assert(string);
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
 * Note: This function can be changed to obtain a compressed byte repsentation of the string.
 */
unsigned int *String_Utils_get_bytes(const char *string) {
    assert(string);
    unsigned int *bytes = malloc(sizeof (unsigned int) * strlen(string));
    int i = 0;
    for (i; i < strlen(string); i++) bytes[i] = (unsigned int) ((unsigned char) string[i]);
    return bytes;
}

/*
 * Properly allocates and deallocates memory.
 */
int String_Utils_equals(const char *string_one, const char *string_two, int parameter) {
    assert(string_one, 0);
    assert(string_two, 0);
    return String_Utils_compare(string_one, string_two, parameter) == 0 ? 1 : 0;
}

/*
 * Properly allocates and deallocates memory.
 */
char *String_Utils_from(char *string, unsigned int index, int parameter) {
    assert(string, NULL);
    int i = index > strlen(string)-1 ? strlen(string)-1 : index;
    int j = 0;
    // Allocate temporary variable.
    char *temp = malloc((strlen(string) + 1) - i); // Note: Uncomplicate the fuck out of this please.
    
    for (i; i <= strlen(string); i++, j++) { // Less complicated
        temp[j] = string[i];
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp, NONE);
        free(temp);
        return string;
    } else return temp;
}

/*
 * Should fix IGNORE_CASE by extracting the portion of the original strange equal to the final string, except
 * in it's normal case. Also I'm trying to inch in uses for my GC.
 *
 * Proud to say that, this should be optimized a lot better than before. The ONLY copies it creates
 * are for string and substring, other than that, no more useless extra allocations and *ugh* memory leaks.
 * I'm improving every day and I can see it.
 */
char *String_Utils_from_token(char *string, const char *substring, int parameter) {
    assert(string, NULL);
    assert(substring, NULL);
    char *temp_string = NULL;
    char *temp_substring = NULL;
    char *temp = NULL;
    char *old_temp = NULL;
    // Allocates a temporary copy of substring depending on parameter passed.
    if(SELECTED(parameter, IGNORE_CASE)){
        temp_string = String_Utils_to_lowercase(string);
        temp_substring = String_Utils_to_lowercase(substring);
        push_strings(vm, 2, temp_string, temp_substring);
        temp = strstr(temp_string, temp_substring);
    } else temp = strstr(string, substring);
    if(temp == NULL) return NULL;
    // here comes the complicated part
    if(SELECTED(parameter, LAST)){
        while(temp != NULL) {
            temp = strstr(temp_string == NULL ? string : temp_string, temp_substring == NULL ? substring, temp_substring);
            // Only time temp_* is NULL is if IGNORE_CASE wasn't passed, hence a check is needed here.
            if(temp != NULL) { // If there is another token, then clearly the previous doesn't count as the last, so...
                // Assigns old_temp to temp
                old_temp = temp; // Set old_temp to the new temp, as it becomes the new 'last'
            }
        }
        temp = old_temp; // Simple, sets temp to old temp.
    }
    temp = String_Utils_substring(string, strlen(string) - strlen(temp), 0, NONE); // Sets temp equal to the same substring except guaranteed to not be lowercase.
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, old_temp, NONE);
        //free everything
        if(temp_string != NULL || temp_substring != NULL) pop_vn(vm, 2);
        free(temp);
        free(old_temp);
        return string;
    }
    // If either temp_* are not null, then IGNORE_CASE was passed, hence pop them from the stack.
    if(temp_string != NULL || temp_substring != NULL) pop_vn(vm, 2);
    return temp;
}

/*
 * CONTINUE HERE!
 */
char **String_Utils_split(char *string, char *delimiter, size_t *size, int parameter) {
    assert(string, NULL);
    assert(delimiter, NULL);
    assert(size, NULL);
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
    assert(string_one, NULL);
    assert(*string_one, NULL);
    assert(string_two, NULL);
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
    assert(string, NULL);
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
    assert(string, NULL);
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
    assert(string, NULL);
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
    assert(array_of_strings, NULL);
    assert(size, NULL);
    assert(delimiter, NULL);
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
    assert(string, 0); assert(find, 0); 
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
    assert(string, 0); assert(find, 0);
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
    assert_VOID(array);
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
    assert(string, NULL);
    char *temp = String_Utils_copy(string, NONE);
    temp[0] = toupper(temp[0]);
    if(SELECTED(parameter, MODIFY)) { String_Utils_set(&string, temp, NONE); free(temp); return string; }
    return temp;
}

/*
 * Correctly allocates and deallocates memory.
 */
char *String_Utils_trim(char *string, int parameter){
    assert(string, NULL);
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
    assert(string, NULL);
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
    assert(string, -1);
    assert(substring, -1);
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
    assert(string, 0);
    assert(substring, 0);
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
    assert(string, NULL);
    assert(start, NULL);
    assert(end, NULL);
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

void String_Utils_Init_GC(void){
    vm = SU_VM_Create();
}

void String_Utils_Destroy_GC(void){
    SU_VM_Destroy(vm);
}