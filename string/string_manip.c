#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "string_manip.h"
#include "../io/logger.h"
#include "../misc/argument_check.h"
#include "../misc/alloc_check.h"

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./string/logs/string_manip.log", "w", C_UTILS_LOG_LEVEL_ALL);


char c_utils_string_char_at(const char *str, unsigned int index) {
    C_UTILS_ARG_CHECK(logger, '\0', str, str && index <= strlen(str));

    return str[index];
}

bool c_utils_string_contains(const char *str, const char *substr, size_t len, bool ignore_case) { 
    C_UTILS_ARG_CHECK(logger, false, str, substr, len >= 0);
    
    size_t str_len = len ? len : strlen(str);
    size_t substr_len = strlen(substr);
    
    if (substr_len > str_len) 
        return false;
    
    if (substr_len == str_len) 
        return c_utils_string_equal(str, substr, len, ignore_case);
    
    for (size_t i = 0; i < str_len - substr_len; i++) {
        char c = str[i];

        if (c == '\0') 
            break;
        
        if (c == substr[0])
            if (c_utils_string_equal(str + i, substr, substr_len, ignore_case))  
                return true;
    }

    return false;
}

char *c_utils_string_lower(char *str, size_t len) {
    C_UTILS_ARG_CHECK(logger, NULL, str, len >= 0);
    
    size_t str_len = len ? len : strlen(str);
    
    for (size_t i = 0; i < str_len; i++) {
        char c = str[i];

        if (c == '\0')
            break;

        // Since tolower takes an int, and returns an int, I ensure (un)signedness by casting.
        str[i] = (char)tolower((unsigned char)c);
    }

    return str;
}

char *c_utils_string_upper(char *str, size_t len) {
    C_UTILS_ARG_CHECK(logger, NULL, str, len >= 0);
    
    size_t str_len = len ? len : strlen(str);
    for (size_t i = 0; i < str_len; i++) {
        char c = str[i];

        if (c == '\0') 
            break;
        
        // Since tolower takes an int, and returns an int, I ensure (un)signedness by casting.
        str[i] = (char)toupper((unsigned char)c);
    }

    return str;
}

bool c_utils_string_equal(const char *string_one, const char *string_two, size_t len, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, false, string_one, string_two);
    
    size_t str_len = len ? len : strlen(string_one);

    if (ignore_case)  
        return strncasecmp(string_one, string_two, str_len) == 0;
    else   
        return strncmp(string_one, string_two, str_len) == 0;
}

char **c_utils_string_split(const char *str, const char *delim, size_t len, size_t *size) {
    C_UTILS_ARG_CHECK(logger, NULL, str, delim);
    
    size_t str_len = len ? len : strlen(str);
    char str_copy[str_len + 1];
    
    snprintf(str_copy, str_len + 1, "%s", str);
    
    char **strs;
    C_UTILS_ON_BAD_MALLOC(strs, logger, sizeof(*strs))
        goto err_strs;

    char *save_ptr, *curr_str = strtok_r(str_copy, delim, &save_ptr);
    if (!curr_str)
        goto err_not_found;

    size_t num_strs = 0;
    do {
        C_UTILS_ON_BAD_REALLOC(&strs, logger, sizeof(*strs) * (num_strs + 1))
            goto err_strs_resize;

        C_UTILS_ON_BAD_MALLOC(strs[num_strs], logger, strlen(curr_str) + 1)
            goto err_curr_str;

        sprintf(strs[num_strs++], "%s", curr_str);
    } while ((curr_str = strtok_r(NULL, delim, &save_ptr)));

    *size = num_strs;
    return strs;

    err_curr_str:
    err_strs_resize:
        for(int i = 0; i < num_strs; i++)
            free(strs[i]);
    err_not_found:
        free(strs);
    err_strs:
        return NULL;
}

char *c_utils_string_reverse(char *str, size_t len) {
    C_UTILS_ARG_CHECK(logger, NULL, str);
    
    size_t str_len = len ? len : strlen(str);
    for (size_t i = 0, j = str_len - 1; i < j; i++, j--) {
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
    }

    return str;
}

char *c_utils_string_replace(char *str, char old_char, char new_char, size_t len, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, NULL, str);
    
    size_t str_len = len ? len : strlen(str);
    char search_char = ignore_case ? tolower((unsigned char) old_char) : old_char;
    for (int i = 0; i < str_len; i++) {
        char curr_char = ignore_case ? tolower((unsigned char) str[i]) : str[i];
        if (curr_char == '\0') 
            break;

        if (search_char == curr_char)  
            str[i] = new_char;
        
    }

    return str;
}

char *c_utils_string_join(const char *arr[], const char *delimiter, size_t size) {
    C_UTILS_ARG_CHECK(logger, NULL, arr, delimiter);
    
    size_t buf_size = BUFSIZ;
    char *buf;
    C_UTILS_ON_BAD_CALLOC(buf, logger, sizeof(*buf))
        goto err_buf;

    size_t allocated = 0, size_left = buf_size;
    for (size_t i = 0; i < size; i++) {
        char *curr_str = (char *) arr[i];
        size_t str_len = strlen(curr_str);

        // If this is not the last iteration, we need to make room for the delimiter.
        if (i != (size - 1))  
            str_len += strlen(delimiter);
        
        while (size_left < str_len) {
            buf_size *= 2;

            C_UTILS_ON_BAD_REALLOC(&buf, logger, buf_size + 1)
                goto err_buf_resize;

            size_left = buf_size - allocated;
        }

        // If this is the last iteration, do not append the delimiter.
        if (i == (size - 1))  
            sprintf(buf, "%s%s", buf, curr_str);
        else   
            sprintf(buf, "%s%s%s", buf, curr_str, delimiter);
        
        allocated += str_len;
        size_left -= str_len;
    }
    
    // Shrink the buffer size to the actual string size.
    C_UTILS_ON_BAD_REALLOC(&buf, logger, strlen(buf) + 1)
        goto err_buf_resize;

    return buf;

    err_buf_resize:
        free(buf);
    err_buf:
        return NULL;
}

bool c_utils_string_starts_with(const char *str, const char *find, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, false, str, find);
    
    size_t str_len = strlen(str), find_len = strlen(find);
    if (find_len > str_len) 
        return false;
    else 
        return c_utils_string_equal(str, find, find_len, ignore_case);
}

bool c_utils_string_ends_with(const char *str, const char *find, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, false, str, find);
    
    size_t str_len = strlen(str), find_len = strlen(find), offset = str_len - find_len;;
    if (find_len > str_len) 
        return false;
    else 
        return c_utils_string_equal(str + offset, find, find_len, ignore_case);
}

char *c_utils_string_trim(char **str_ptr, size_t len) {
    C_UTILS_ARG_CHECK(logger, NULL, str_ptr, str_ptr && *str_ptr);
    
    char *str = *str_ptr;
    size_t str_len = len ? len : strlen(str);
    
    char *buf;
    C_UTILS_ON_BAD_MALLOC(buf, logger, str_len + 1)
        goto err;

    size_t i = 0, j = str_len - 1;
    for (; i < str_len; i++)  
        if (!isspace((unsigned char)str[i])) 
            break;
    
    for (; j > i; j--)  
        if (!isspace((unsigned char)str[j])) 
            break;
    
    /* 
        We must now copy the string from the offset past any empty spaces at the beginning and before the end.
        +2 because +1 for null terminator, and +1 because we start at index 0 for j, so we offset for it.
    */
    snprintf(buf, j - i + 2, "%s", str + i);
    
    // Now we shrink it to the new size.
    C_UTILS_ON_BAD_REALLOC(&buf, logger, strlen(buf) + 1)
        goto err;

    *str_ptr = buf;
    return buf;

    err:
        free(buf);
        return NULL;
}

char *c_utils_string_substring(const char *str, unsigned int offset, unsigned int end) {
    C_UTILS_ARG_CHECK(logger, NULL, str);

    if (end && offset > end)
        return NULL;
    
    size_t new_end = end ? end : strlen(str);
    size_t buf_size = (new_end - offset) + 1;
    char *buf;
    C_UTILS_ON_BAD_MALLOC(buf, logger, buf_size + 1);
        goto err;

    snprintf(buf, buf_size, "%s", str + offset);
    // In case a null terminator was found, it will shrink the buffer to actual size.
    C_UTILS_ON_BAD_REALLOC(&buf, logger, strlen(buf) + 1)
        goto err;

    return buf;

    err:
        free(buf);
        return NULL;
}

long long int c_utils_string_index_of(const char *str, const char *substr, size_t len, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, -1, str, substr);
    
    size_t substr_len = strlen(substr), str_len = len ? len : strlen(str);
    if (substr_len > str_len) 
        return -1;

    if (substr_len == str_len)
        return c_utils_string_equal(str, substr, len, ignore_case) ? 0 : -1;

    for (size_t i = 0; i < str_len - substr_len; i++) {
        char c = ignore_case ? tolower((unsigned char)str[i]) : str[i];
        if (c == '\0') 
            break;
        
        if (c == ignore_case ? tolower((unsigned char)substr[0]) : substr[0])
            if (c_utils_string_equal(str + i, substr, substr_len, ignore_case))
                return i;
    }

    return -1;
}

unsigned int c_utils_string_count(const char *str, const char *substr, size_t len, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, 0, str, substr);
    
    size_t str_len = len ? len : strlen(str), substr_len = strlen(substr), count = 0;
    if (substr_len > str_len) 
        return 0;

    if (substr_len == str_len)
        return !!c_utils_string_equal(str, substr, str_len, ignore_case);
    
    for (size_t i = 0; i < str_len - substr_len; i++) {
        char c = str[i];
        if (c == '\0') 
            break;

        if (c == substr[0])
            if (c_utils_string_equal(str + i, substr, substr_len, ignore_case)) {
                count++;
                i += (substr_len - 1);
            }
    }

    return count;
}

// Reimplement function to use strcasestr().
char *c_utils_string_between(const char *str, const char *start, const char *end, size_t len, bool ignore_case) {
    C_UTILS_ARG_CHECK(logger, NULL, str, start, end);
    
    size_t str_len = len ? len : strlen(str), start_len = strlen(start), end_len = strlen(end);
    char buf[str_len + 1];
    snprintf(buf, str_len, "%s", str);
    
    c_utils_string_lower(buf, str_len);
    char *start_str = strstr(buf, start);
    if (!start_str) 
        return NULL;
    
    size_t start_offset = (str_len - strlen(start_str)) + start_len;
    char *end_str = strstr(buf + start_offset, end);
    if (!end_str) 
        return NULL;
    
    size_t end_offset = (str_len - strlen(end_str)) - end_len;
    size_t new_len = end_offset - start_offset;
    char *substr;
    C_UTILS_ON_BAD_MALLOC(substr, logger, new_len + 1)
        return NULL;
    snprintf(substr, new_len, "%s", str + start_offset);
    
    return substr;
}

void c_utils_string_destroy(char **str_ptr) {
    free(*str_ptr);
}
