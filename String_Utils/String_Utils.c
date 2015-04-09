#include "String_Utils.h"

char *String_Utils_concat(char *string_one, char *string_two, int parameter){
    char *temp = malloc(strlen(string_one) + strlen(string_two));
    strcat(temp, string_one);
    strcat(temp, string_two);
    // Based on parameters below, will modify the original screen, or just return the new string.
    if(parameter == MODIFY){
        string_one = realloc(string_one, strlen(temp));
        strcpy(string_one, temp);
        return string_one;  
    } else return temp; // Not modifying the original string is the default, even if there was no valid parameter.
}

void String_Utils_update(String_Utils *self){ // IMPLEMENT!
    printf("Not Implemented!");
}


int String_Utils_compare(char *string_one, char *string_two, int parameter){
    if(parameter == IGNORE_CASE) return strcmp(String_Utils_to_lowercase(string_one, NO_MODIFY), String_Utils_to_lowercase(string_two, NO_MODIFY));
    else return strcmp(string_one, string_two);
}


char String_Utils_char_at(char *string, unsigned int index){
    return string[index > strlen(string) ? strlen(string) - 1 : index];
}

int String_Utils_contains(char *string, char *search, int parameter){ // FIX!
    if(parameter == IGNORE_CASE) return strstr(String_Utils_to_lowercase(string, NO_MODIFY), String_Utils_to_lowercase(search, NO_MODIFY)) == NULL ? 0 : 1;
    else return strstr(string, search) == NULL ? 0 : 1;
}

char *String_Utils_to_lowercase(char *string, int parameter){
    char *temp = malloc(strlen(string));
    int i = 0;
    for(i; i < strlen(string); i++){
        temp[i] = tolower(string[i]);
    }
    if(parameter = MODIFY){
        strcpy(string, temp);
        return string;
    } else return temp;
}

unsigned int *String_Utils_get_bytes(char *string){
    unsigned int *bytes = malloc(sizeof(unsigned int) * strlen(string));
    int i = 0;
    for(i; i < strlen(string); i++) bytes[i] = (unsigned int)((unsigned char)string[i]);
    return bytes;
}

int String_Utils_equals(char *string_one, char *string_two, int parameters){
    return String_Utils_compare(string_one, string_two, parameters) == 0 ? 1 : 0;
}

String_Utils *String_Utils_create(void){
    String_Utils *string = malloc(sizeof(String_Utils));
    string->concat = String_Utils_concat;
    string->compare = String_Utils_compare;
    string->update = String_Utils_update;
    string->char_at = String_Utils_char_at;
    string->contains = String_Utils_contains;
    string->get_bytes = String_Utils_get_bytes;
    string->equals = String_Utils_equals;
    return string;
}