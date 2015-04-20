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
    char *temp;
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
        char *temp_string_one = String_Utils_to_lowercase(string_one, NONE);
        char *temp_string_two = String_Utils_to_lowercase(string_two, NONE);
        push_strings(vm, 2, temp_string_one, temp_string_two); // Puts both elements on stack.
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
        char *temp_string = strdup(string);
        char *temp_search = strdup(search);
        push_strings(vm, 2, temp_string, temp_search); // Puts both elements on stack.
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
        String_Utils_set(&string, temp);
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
        String_Utils_set(&string, temp);
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
    assert(string_one);
    assert(string_two);
    return String_Utils_compare(string_one, string_two, parameter) == 0 ? 1 : 0;
}

/*
 * Properly allocates and deallocates memory.
 */
char *String_Utils_from(char *string, unsigned int index, int parameter) {
    assert(string);
    int i = index > strlen(string)-1 ? strlen(string)-1 : index;
    int j = 0;
    // Allocate temporary variable.
    char *temp = malloc((strlen(string) + 1) - i); // Note: Uncomplicate the fuck out of this please.
    
    for (i; i <= strlen(string); i++, j++) { // Less complicated
        temp[j] = string[i];
    }
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp);
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
    assert(string);
    assert(substring);
    char *temp_string = NULL;
    char *temp_substring = NULL;
    char *temp = NULL;
    char *old_temp = NULL;
    // Allocates a temporary copy of substring depending on parameter passed.
    if(SELECTED(parameter, IGNORE_CASE)){
        temp_string = String_Utils_to_lowercase(string, NONE);
        temp_substring = String_Utils_to_lowercase(substring, NONE);
        push_strings(vm, 2, temp_string, temp_substring);
        temp = strstr(temp_string, temp_substring);
    } else temp = strstr(string, substring);
    if(temp == NULL) return NULL;
    // here comes the complicated part
    if(SELECTED(parameter, LAST)){
        while(temp != NULL) {
            temp = strstr(temp, temp_substring == NULL ? substring : temp_substring);
            // Only time temp_* is NULL is if IGNORE_CASE wasn't passed, hence a check is needed here.
            if(temp != NULL) { // If there is another token, then clearly the previous doesn't count as the last, so...
                // Assigns old_temp to temp
                old_temp = temp; // Set old_temp to the new temp, as it becomes the new 'last'
                if(strlen(temp) <= strlen(substring)) temp = NULL;
                else temp += strlen(substring);
            }
        }
        temp = old_temp; // Simple, sets temp to old temp.
    }
    temp = String_Utils_substring(string, strlen(string) - strlen(temp), strlen(string), NONE); // Sets temp equal to the same substring except guaranteed to not be lowercase.
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, old_temp);
        //free everything
        if(temp_string != NULL || temp_substring != NULL) pop_vn(vm, 2);
        free(temp);
        //free(old_temp);
        return string;
    }
    // If either temp_* are not null, then IGNORE_CASE was passed, hence pop them from the stack.
    if(temp_string != NULL || temp_substring != NULL) pop_vn(vm, 2);
    return temp;
}

/*
 * Should be optimized so as to not allocate an extra cell in the array. Also, edged in another GC.
 * Should no longer attempt to free twice!
 */
char **String_Utils_split(const char *string, const char *delimiter, size_t *size) {
    assert(string);
    assert(delimiter);
    assert(size);
    // Allocates an array of strings, initial size of only one string.
    char **string_array = malloc(sizeof (char *));
    char *temp = NULL;
    // Allocated a temporary string so original string is not modified from strtok.
    char *temp_string = strdup(string);
    temp = strtok(temp_string, delimiter); // So strtok does not mess with the original string.
    if (temp == NULL) { free(temp_string); return NULL; } // If temp does not contain the delimiter, then free the tmep_string and return NULL.
    push_strings(vm, 1, temp_string); // Push both temp and temp_string on the stack.
    unsigned int index = 0; // Used to record the size of the array.
    while (temp != NULL) {
        // If the index is not 0, a simple check so as to not waste an extra allocation, then reallocate the array to be the
        // index + 1, which is just enough for string_array[index].
        if(index != 0) string_array = realloc(string_array, (sizeof (char *) * (index + 1))); 
        // The index in the array is allocated
        string_array[index] = malloc(strlen(temp) + 1);
        strcpy(string_array[index], temp);
        index++;
        // Reallocates the array so it can hold another array.
        temp = strtok(NULL, delimiter);
    }
    pop_vn(vm, 1);
    *size = index;
    return string_array;
}

/*
 * Majorly optimized down to a two-step operation (besides for assertions for null).
 */
char *String_Utils_set(char **string_one, const char *string_two) {
    assert(string_one);
    assert(*string_one);
    assert(string_two);
    free(*string_one); // Will crash on a string literal.
    *string_one = strdup(string_two);
    return *string_one;
}

/*
 * Now keeps track of the old values of temp by pushing all of temp's values on the stack, then popping them all off.
 * A lot more optimized compared to the previous. Also, a fairly good excuse to use my GC.
 */
char *String_Utils_concat_all(int parameter, size_t amount, char *string, ...) {
    assert(string);
    va_list args;
    int i = 0;
    va_start(args, string);
    char *final_string = strdup(string); // final_string starts with the very first string.
    char *temp = NULL;
    for(i; i < amount; i++){
        temp = strdup(va_arg(args, char *)); // This loses the old value of temp, so temp's value is kept on the stack to be deallocated.
        push_string(vm, temp); // Push new string on stack.
        final_string = String_Utils_concat(final_string, temp, MODIFY);
    }
    va_end(args);
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, final_string);
        free(final_string);
        pop_vn(vm, amount);
        return string;
    } else { pop_vn(vm, amount); return final_string; } // Pops the temp values off of the stack.
}

/*
 * For loop should correctly reverse the string without underflowing like before.
 */
char *String_Utils_reverse(char *string, int parameter) {
    assert(string);
    char *temp = malloc(strlen(string) + 1);
    int i, j;
    for (i = 0, j = strlen(string) - 1; i < strlen(string); i++, j--) {
        temp[i] = string[j];
    }
    temp[strlen(string)] = '\0';
    if (SELECTED(parameter, MODIFY)) {
        String_Utils_set(&string, temp);
        free(temp);
        return string;
    } else return temp;
}

/*
 * Trimmed the redundant for loops for SELECTED statement into one for loop
 */
char *String_Utils_replace(char *string, char old_char, char new_char, int parameter){
    assert(string);
    char *temp = malloc(strlen(string) + 1);
    int i = 0;
    for(i; i <= strlen(string); i++){
        // if IGNORE_CASE is selected, then compare the lower case chars, else the normal case chars.
        // tolower(string[i]) == tolower(old_char) or string[i] == old_char
        if((SELECTED(parameter, IGNORE_CASE) ? tolower(string[i]) : string[i] ) ==
           (SELECTED(parameter, IGNORE_CASE) ? tolower(old_char) : old_char)) temp[i] = new_char;
        else temp[i] = string[i];
    }
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, temp);
        free(temp);
        return string;
    } else return temp;
}

/*
 * Changed size_t to not use a pointer when it doesn't even modify it.
 */
char *String_Utils_join(const char **array_of_strings, const char *delimiter, size_t size){
    assert(array_of_strings);
    assert(delimiter);
    char *temp = NULL;
    int i = 0;
    for(i; i < size; i++){
        if(temp == NULL) temp = strdup(array_of_strings[i]);
        else temp = String_Utils_concat_all(MODIFY, 2, temp, delimiter, array_of_strings[i]);
    }
    return temp;
}

/*
 * At a impasse here for a solution. Either I allocate to save efficiency by only checking
 * for lowercase once at the expense of allocating memory... or checking every pass of the
 * for loop, wasting resource on a bitwise operation to check for parameter, yet no extra
 * memory allocation.
 */
int String_Utils_starts_with(const char *string, const char *find, int parameter){
    assert(string);
    assert(find);
    int i = 0;
    for(i; i < strlen(find); i++) if(SELECTED(parameter, IGNORE_CASE)){
        if(tolower(string[i]) != tolower(find[i])) return 0;
    } else if(string[i] != find[i]) return 0;
    return 1;
}

/*
 * Same problem as starts_with, impasse.
 */
int String_Utils_ends_with(const char *string, const char *find, int parameter){
    assert(string);
    assert(find);
    int i, j;
    for(i = strlen(string) - strlen(find), j = 0; i < strlen(string); i++, j++){
        if(SELECTED(parameter, IGNORE_CASE)){
            if(tolower(string[i]) != tolower(find[j])) return 0; 
        } else if(string[i] != find[j]) return 0;
    }
    return 1;
}


/*
 * Should work as intended without memory leaks. 
 */
char *String_Utils_capitalize(char *string, int parameter){
    assert(string);
    char *temp = strdup(string);
    temp[0] = toupper(temp[0]);
    if(SELECTED(parameter, MODIFY)) { String_Utils_set(&string, temp); free(temp); return string; }
    return temp;
}

/*
 * Correctly allocates and deallocates memory.
 */
char *String_Utils_trim(char *string, int parameter){
    assert(string);
    char *temp = NULL;
    int i = 0;
    int j = strlen(string)-1;
    for(i; i < strlen(string); i++){
        if(!isspace(string[i])) break;
    }
    for(j; j > i ; j--){
        if(!isspace(string[j])) break;
    }
    temp = String_Utils_substring(string, i, j, NONE);
    if(SELECTED(parameter, MODIFY)){
        String_Utils_set(&string, temp);
        free(temp);
        return string;
    }
    return temp;
}

/*
 * Removed old_temp as it's useless now that copy is gone.
 */
char *String_Utils_substring(char *string, unsigned int begin, unsigned int end, int parameter){
    assert(string);
    // If begin is greater than the end, set new_begin to 0, else equal to begin
    unsigned int new_begin = begin > end ? 0 : begin; // Bounds checking for begin
    // If end is out of bounds, set new_end to the length of the string - 1, else equal to end
    unsigned int new_end = end >= strlen(string) ? strlen(string) - 1 : end;
    size_t size = new_end - new_begin; // The size of the substring will be end - begin
    char *temp = malloc(size + 1); // Allocate a temporary variable to hold substring.
    //memset(temp, 0, size + 1); // Zeroes the struct to clean it.
    memcpy(temp, string + begin, size + 1); // Copy into temp (destination), the contents of string[begin] size bytes.
    temp[size + 1] = '\0'; // Append a null terminator to the temporary string.
    if(SELECTED(parameter, MODIFY)) { // Modifies the original string is parameter is passed.
        String_Utils_set(&string, temp);
        free(temp); 
        return string; 
    }
    return temp;
}

/*
 * Still over complicated, but slightly less so, also more optimized as no longer makes copies of strings 
 * meaninglessly, but it does not make checks on each loop, which has to be optimized out at a later date.
 */
int String_Utils_index_of(const char *string, const char *substring, int parameter){
    assert(string);
    assert(substring);
    char *temp = NULL;
    char *temp_string = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(string, NONE) : NULL;
    char *temp_substring = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(substring, NONE) : NULL;
    if (temp_string != NULL && temp_substring != NULL) push_strings(vm, 2, temp_string, temp_substring);
    char *old_temp = NULL;
    temp = strstr(temp_string == NULL ? string : temp_string, temp_substring == NULL ? substring : temp_substring);
    if(temp == NULL) {
        if (temp_string != NULL && temp_substring != NULL) pop_vn(vm, 2);
        return 0; 
    }
    if(SELECTED(parameter, LAST)){
        while(temp != NULL) {
            temp = strstr(temp_string == NULL ? string : temp_string, temp_substring == NULL ? substring : temp_substring);
            if(temp != NULL){
                // Free old_temp before assigning it to copy of temp.
                old_temp = temp; 
            }
        }
        temp = old_temp;
    }
    int result = strlen(string) - strlen(temp);
    if (temp_string != NULL && temp_substring != NULL) pop_vn(vm, 2);
    return result;
}

/*
 * Fixed a few things, but not 100% sure it'll work, gotta watch this.
 */
int String_Utils_count(const char *string, const char *substring, int parameter){
    assert(string);
    assert(substring);
    int count = 0;
    char *temp = NULL; 
    char *temp_ptr = NULL;
    temp = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(string, NONE) : string;
    temp_ptr = temp; // For when temp gets set to NULL, will be able to keep track of it to be freed later.
    while(temp = strstr(temp, substring)){ // temp gets set to the next occurrence of the found substring 
        count++;
        //printf("Count is %d\nCurrent string being compared: %s\nString length = %d\nSubstring length: %d\n", count, string, strlen(string), strlen(substring));
        if(strlen(temp) < strlen(substring)) temp = NULL;
        else temp += strlen(substring);
        //printf("New String: %s\nNew String Length: %d\n", temp, strlen(temp));
    }
    if(SELECTED(parameter, IGNORE_CASE)) free(temp_ptr); // If IGNORE_CASE was passed, then it's safe to free the copy of temp.
    return count;
}

/*
 * Jesus, this function is 1000% over complicated, but it definitely demonstrates a good reason to use my
 * GC. Instead of having to redundantly free everything, I can just add all 4 to the stack in one line, and
 * pop them all in one line as well.
 */
char *String_Utils_between(const char *string, const char *start, const char *end, int parameter){
    assert(string);
    assert(start);
    assert(end);
    char *temp = NULL; 
    char *new_temp = NULL;
    char *temp_string = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(string, NONE) : string;
    char *temp_start = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(start, NONE) : start;
    char *temp_end = SELECTED(parameter, IGNORE_CASE) ? String_Utils_to_lowercase(end, NONE) : end; 
    temp = strstr(temp_string, temp_start) + strlen(temp_start);
    if(SELECTED(parameter, IGNORE_CASE)) push_strings(vm, 4, temp_string, temp_start, temp_end, temp);
    if(temp == NULL) { if(SELECTED(parameter, IGNORE_CASE)) pop_vn(vm, 4); return NULL;}
    size_t size_of_substring = strlen(temp) - (strlen(strstr(temp_string, temp_end)));
    size_t index_of_start = strlen(string) - strlen(temp);
    new_temp = String_Utils_substring(string, index_of_start, size_of_substring, NONE);
    if(SELECTED(parameter, IGNORE_CASE)) pop_vn(vm, 4);
    return new_temp;
}

/*
* Required to initialize the garbage collector. Too bad it can't be static or called automatically.
*/
void String_Utils_Init_GC(void){
    vm = SU_VM_Create();
}
/*
* Destroys the virtual machine, which also frees everything on the stack preventing memory leaks as long as
* this is called.
*/
void String_Utils_Destroy_GC(void){
    SU_VM_Destroy(vm);
}