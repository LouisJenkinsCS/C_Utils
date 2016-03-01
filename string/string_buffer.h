#ifndef C_UTILS_STRING_BUFFER_H
#define C_UTILS_STRING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#define _GNU_SOURCE
#include <stdio.h>

/*
	A string buffer is an automatically/dynamically resizing buffer which holds a string.
	It automatically resizes when appending newer items, and increased by a factor of 1.5 each time.

	This will allow much easier appending and prepending of data to streams of characters (a string),
	without the necessity of having to keep track of sprintf or asprintf and resizing it on your own.

	It allows for simplified generic types, which can be of any of the standard types. String literals
	MUST be put through the non generic function, as it will fit perfectly anyway.
*/
struct c_utils_string_buffer;

#define C_UTILS_GENERIC_FORMAT(x) _Generic((x), \
    char: "%c", \
    signed char: "%hhd", \
    unsigned char: "%hhu", \
    signed short: "%hd", \
    unsigned short: "%hu", \
    signed int: "%d", \
    unsigned int: "%u", \
    long int: "%ld", \
    unsigned long int: "%lu", \
    long long int: "%lld", \
    unsigned long long int: "%llu", \
    float: "%f", \
    double: "%f", \
    long double: "%Lf", \
    char *: "%s", \
   	const char *: "%s", \
    void *: "%p" )

#define C_UTILS_STRING_BUFFER_APPEND_FORMAT(buf, format, ...) do { \
	char *str; \
	asprintf(&s, format, ##__VA_ARGS__); \
	c_utils_string_buffer_append(buf, str); \
	free(str); \
} while(0)

#define C_UTILS_STRING_BUFFER_APPEND(buf, val) C_UTILS_STRING_BUFFER_APPEND_FORMAT(buf, C_UTILS_GENERIC_FORMAT(val), val)

#define C_UTILS_STRING_BUFFER_PREPEND_FORMAT(buf, format, ...) do { \
	char *str; \
	asprintf(&s, format, ##__VA_ARGS__); \
	c_utils_string_buffer_prepend(buf, str); \
	free(str); \
} while(0)

#define C_UTILS_STRING_BUFFER_PREPEND(buf, val) C_UTILS_STRING_BUFFER_PREPEND_FORMAT(buf, C_UTILS_GENERIC_FORMAT(val), val)

#define C_UTILS_STRING_BUFFER_INSERT_FORMAT(buf, format, start, end, ...) do { \
	char *str; \
	asprintf(&s, format, ##__VA_ARGS__); \
	c_utils_string_buffer_insert(buf, str, start, end); \
	free(str); \
} while(0)

#define C_UTILS_STRING_BUFFER_INSERT(buf, val, start, end) C_UTILS_STRING_BUFFER_INSERT_FORMAT(buf, C_UTILS_GENERIC_FORMAT(val), start, end, val)

#define C_UTILS_STRING_BUFFER_END -1

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_string_buffer string_buffer_t;

/*
	Macros
*/
#define STRING_BUFFER_APPEND(...) C_UTILS_STRING_BUFFER_APPEND(__VA_ARGS__)
#define STRING_BUFFER_PREPEND(...) C_UTILS_STRING_BUFFER_PREPEND(__VA_ARGS__)
#define STRING_BUFFER_INSERT(...) C_UTILS_STRING_BUFFER_INSERT(__VA_ARGS__)
#define STRING_BUFFER_APPEND_FORMAT(...) C_UTILS_STRING_BUFFER_APPEND_FORMAT(__VA_ARGS__)
#define STRING_BUFFER_PREPEND_FORMAT(...) C_UTILS_STRING_BUFFER_PREPEND_FORMAT(__VA_ARGS__)
#define STRING_BUFFER_INSERT_FORMAT(...) C_UTILS_STRING_BUFFER_INSERT_FORMAT(__VA_ARGS__)
#define STRING_BUFFER_END C_UTILS_STRING_BUFFER_END

/*
	Functions
*/
#define string_buffer_create(...) c_utils_string_buffer_create(__VA_ARGS__)
#define string_buffer_append(...) c_utils_string_buffer_append(__VA_ARGS__)
#define string_buffer_prepend(...) c_utils_string_buffer_prepend(__VA_ARGS__)
#define string_buffer_insert(...) c_utils_string_buffer_insert(__VA_ARGS__)
#define string_buffer_reverse(...) c_utils_string_buffer_reverse(__VA_ARGS__)
#define string_buffer_size(...) c_utils_string_buffer_size(__VA_ARGS__)
#define string_buffer_delete(...) c_utils_string_buffer_delete(__VA_ARGS__)
#define string_buffer_clear(...) c_utils_string_buffer_clear(__VA_ARGS__)
#define string_buffer_get(...) c_utils_string_buffer_get(__VA_ARGS__)
#define string_buffer_take(...) c_utils_string_buffer_take(__VA_ARGS__)
#define string_buffer_substring(...) c_utils_string_buffer_substring(__VA_ARGS__)
#define string_buffer_beyond(...) c_utils_string_buffer_beyond(__VA_ARGS__)
#define string_buffer_before(...) c_utils_string_buffer_before(__VA_ARGS__)
#define string_buffer_destroy(...) c_utils_string_buffer_destroy(__VA_ARGS__)
#endif

/*
	Creates a string buffer with the passed string as it's initial value, as
	well as configure it's initial size based on it. If synchronized, it will
	have a RWLock to allow an easier producer-consumer relationship.

	Hence, passing "Hello World" will allocate 12 bytes plus 6, making 18 bytes.

	If str is NULL, then it will have an initial size of 126.
*/
struct c_utils_string_buffer *c_utils_string_buffer_create(char *str, bool synchronized);

/*
	Appends the string to the string buffer. If str or buf is NULL, it will return false, otherwise true.
	The string buffer will resize if needed.
*/
bool c_utils_string_buffer_append(struct c_utils_string_buffer *buf, char *str);

/*
	Prepends the string to the string buffer. If the str or buf is NULL, it will return false, otherwise true.
	The string buffer will resize if needed.
*/
bool c_utils_string_buffer_prepend(struct c_utils_string_buffer *buf, char *str);

/*
	Inserts the string inside the string buffer at the requested index. The function will do nothing if
	either start or end is greater than the currently used portions of the string. If (start - end) > size,
	it will resize the string buffer as needed.

	If buf or str is NULL, or if start > end, or if start and/or end is greater than currently used size,
	it will return false, otherwise true.
*/
bool c_utils_string_buffer_insert(struct c_utils_string_buffer *buf, char *str, int start, int end);

/*
	Reverses the data held by this string buffer.

	If buf is NULL, returns false.
*/
bool c_utils_string_buffer_reverse(struct c_utils_string_buffer *buf);

/*
	Returns the size of the data held by this string buffer.

	If buf is NULL, returns -1.
*/
int c_utils_string_buffer_size(struct c_utils_string_buffer *buf);

/*
	Deletes the portions of the string buffer between indexes start and end.

	If buf or str is NULL, or if start > end, or if start and/or end is greater than currently used size,
	it will return false, otherwise true.
*/
bool c_utils_string_buffer_delete(struct c_utils_string_buffer *buf, int start, int end);

/*
	Clears the buffer of all data.

	If buf is NULL, returns false, otherwise true.
*/
bool c_utils_string_buffer_clear(struct c_utils_string_buffer *buf);

/*
	Retrieves the data held by this string buffer.

	If buf is NULL, returns NULL.
*/
char *c_utils_string_buffer_get(struct c_utils_string_buffer *buf);

/*
	Retrieves the data held by this string buffer and removed it from the buffer.

	If buf is NULL, returns NULL.
*/
char *c_utils_string_buffer_take(struct c_utils_string_buffer *buf);

/*
	Retrieves the data between indexes start and end held by this string buffer.

	If buf is NULL, or start and/or end is out of bounds, returns NULL.
*/
char *c_utils_string_buffer_substring(struct c_utils_string_buffer *buf, int start, int end);

/*
	Retrieves the data past the given index. 

	If buf is NULL, or after is out of bounds, returns NULL.
*/
char *c_utils_string_buffer_beyond(struct c_utils_string_buffer *buf, int after);

/*
	Retrieves the data before the given index.

	If buf is NULL, or before is out of bounds, returns NULL.
*/
char *c_utils_string_buffer_before(struct c_utils_string_buffer *buf, int before);

/*
	Destroys the buffer, clearing it all of all contents first.
*/
void c_utils_string_buffer_destroy(struct c_utils_string_buffer *buf);

#endif /* C_UTILS_STRING_BUFFER_H */