#ifndef NET_UTILS_HTTP_H
#define NET_UTILS_HTTP_H

#include <stddef.h>
#include <stdbool.h>

enum c_utils_http_method {
    /// If the method is unitialized
    C_UTILS_HTTP_NO_METHOD,
    C_UTILS_HTTP_GET,
    C_UTILS_HTTP_POST,
    C_UTILS_HTTP_HEAD,
    C_UTILS_HTTP_DELETE,
    C_UTILS_HTTP_TRACE,
    C_UTILS_HTTP_CONNECT
};

enum c_utils_http_version {
    /// If the version is uninitialized.
    C_UTILS_HTTP_NO_VER,
    C_UTILS_HTTP_VER_1_0,
    C_UTILS_HTTP_VER_1_1,
    C_UTILS_HTTP_VER_1_X
};

struct c_utils_response {
    /// Contains the fields parsed from the header in a hash table.
    struct c_utils_map *header;
    /// The HTTP version.
    enum c_utils_http_version version;
    /// The HTTP status.
    unsigned int status;
};

struct c_utils_request {
    /// Contains the fields parsed from the header in a hash table.
    struct c_utils_map *header;
    /// The HTTP method.
    enum c_utils_http_method method;
    /// The request file path.
    char *path;
    /// The HTTP version.
    enum c_utils_http_version version;
};

struct c_utils_field {
    char *field;
    char *value;
};

#ifdef NO_C_UTILS_PREFIX
/*
    Typedefs
*/
typedef struct c_utils_response response_t;
typedef struct c_utils_request request_t;
typedef enum c_utils_http_method http_method_e;
typedef enum c_utils_http_version http_version_e;

/*
    Prefix-Stripped Functions  
*/
#define request_create(...) c_utils_request_create(__VA_ARGS__)
#define response_create(...) c_utils_response_create(__VA_ARGS__)
#define request_clear(...) c_utils_request_clear(__VA_ARGS__)
#define response_clear(...) c_utils_response_clear(__VA_ARGS__)
#define request_set_field(...) c_utils_request_set_field(__VA_ARGS__)
#define response_set_field(...) c_utils_response_set_field(__VA_ARGS__)
#define request_get_field(...) c_utils_request_get_field(__VA_ARGS__)
#define response_get_field(...) c_utils_response_get_field(__VA_ARGS__)
#define request_append(...)
#define response_append(...)
#define request_to_string(...) c_utils_request_to_string(__VA_ARGS__)
#define response_to_string(...) c_utils_response_to_string(__VA_ARGS__)
#define REQUEST_WRITE(...) C_UTILS_REQUEST_WRITE(__VA_ARGS__)
#define RESPONSE_WRITE(...) C_UTILS_RESPONSE_WRITE(__VA_ARGS__)

/*
    Prefix-Stripped Enumerators
*/

// HTTP Methods
#define HTTP_NO_METHOD C_UTILS_HTTP_NO_METHOD
#define HTTP_GET C_UTILS_HTTP_GET
#define HTTP_POST C_UTILS_HTTP_POST
#define HTTP_HEAD C_UTILS_HTTP_HEAD
#define HTTP_DELETE C_UTILS_HTTP_DELETE
#define HTTP_TRACE C_UTILS_HTTP_TRACE
#define HTTP_CONNECT C_UTILS_HTTP_CONNECT

// HTTP Versions
#define HTTP_NO_VER C_UTILS_HTTP_NO_VER
#define HTTP_VER_1_0 C_UTILS_HTTP_VER_1_0
#define HTTP_VER_1_1 C_UTILS_HTTP_VER_1_1
#define HTTP_VER_1_X C_UTILS_HTTP_VER_1_X
#endif /* NO_C_UTILS_PREFIX */

/*
    Adjustable maximum lengths
*/

#ifdef C_UTILS_HTTP_FILE_PATH_MAX_LEN
#define C_UTILS_HTTP_FILE_PATH_LEN C_UTILS_HTTP_FILE_PATH_MAX_LEN
#else
#define C_UTILS_HTTP_FILE_PATH_LEN 128
#endif

#ifdef C_UTILS_HTTP_HEADER_FIELD_MAX_LEN
#define C_UTILS_HTTP_HEADER_FIELD_LEN C_UTILS_HTTP_HEADER_FIELD_MAX_LEN
#else
#define C_UTILS_HTTP_HEADER_FIELD_LEN 128
#endif

#ifdef C_UTILS_HTTP_HEADER_VALUE_MAX_LEN
#define C_UTILS_HTTP_HEADER_VALUE_LEN C_UTILS_HTTP_HEADER_VALUE_MAX_LEN
#else
#define C_UTILS_HTTP_HEADER_VALUE_LEN 1024
#endif

#ifdef C_UTILS_HTTP_HEADER_MAX_LEN
#define C_UTILS_HTTP_HEADER_LEN C_UTILS_HTTP_HEADER_MAX_LEN
#else
#define C_UTILS_HTTP_HEADER_LEN 4096
#endif

// TODO: Make Field Values case-insensitive!

/**
 * Creates an empty response object. It will not contain any fields, nor any attributes.
 * @return New empty c_utils_response object.
 */
struct c_utils_response *c_utils_response_create(void);

/**
 * Creates an empty request object. It will not contain any fields, nor any attributes.
 * @return New empty c_utils_response object.
 */
struct c_utils_request *c_utils_request_create(void);

/*
    NOTE: When parsing from header, make sure to NOT read past the header_size. Try to make sure header is of appropriate size.
    This will append what it can to the mapped header, and return what it cannot. I.E After end of header, or incomplete portions.
    header_size will be the size of what is left (I.E what is invalid or not part of the header). Response should be cleared before passing it!
    TODO: Unit Test this with a purposely invalid header, and with an incomplete header, and without a header.
*/

/**
 * Appends and parses the header to the response. The header_size must intially contain the actual size of
 * the header, and will return the offset of which it stopped reading as well as the offset pointer.
 * @param res Instance of response.
 * @param header Header to parse and append.
 * @param header_size Used to obtain size of header, and return the size of the header.
 * @return What could not be read, meaning the beginning of the null terminator on success, otherwise start of character could not parse.
 */
char *c_utils_response_append(struct c_utils_response *res, const char *header, size_t *header_size);

/**
 * Appends and parses the header to the request. The header_size must intially contain the actual size of
 * the header, and will return the offset of which it stopped reading as well as the offset pointer.
 * @param req Instance of request.
 * @param header Header to parse and append.
 * @param header_size Used to obtain size of header, and return the size of the header.
 * @return What could not be read, meaning the beginning of the null terminator on success, otherwise start of character could not parse.
 */
char *c_utils_request_append(struct c_utils_request *req, const char *header, size_t *header_size);

/**
 * Clears the header of mapped values.
 * @param res Response.
 * @return If Successful.
 */
bool c_utils_response_clear(struct c_utils_response *res);

/**
 * Clears the header of mapped values.
 * @param req Request.
 * @return If Successful.
 */
bool c_utils_request_clear(struct c_utils_request *req);

/**
 * Returns the string representation of the parsed/generated header.
 * @param res Response.
 * @return If Successful.
 */
char *c_utils_response_to_string(struct c_utils_response *res);

/**
 * Returns the string representation of the parsed/generated header.
 * @param req Request.
 * @return If Successful.
 */
char *c_utils_request_to_string(struct c_utils_request *req);

/**
 * A helper-macro used for convenience which allows you to declare key-value pairs in the guise of an anonymous struct declaration, I.E
 * C_UTILS_RESPONSE_WRITE(res, 200, C_UTILS_HTTP_VER_1_0, { "Content-Length", "100" }, { "Content-Type", "text/html" });
 * @param response Response.
 * @param http_status The HTTP status.
 * @param http_version The HTTP version.
 * @param ... Variadic arguments of field types, cannot be empty, but can be NULL.
 */
#define C_UTILS_RESPONSE_WRITE(response, http_status, http_version, ...) do { \
    struct c_utils_field *fields = (struct c_utils_field[]){ __VA_ARGS__ }; \
    size_t arr_size = sizeof((struct c_utils_field[]){ __VA_ARGS__ }) / sizeof(struct c_utils_field); \
    int i = 0; \
    response->status = http_status; \
    response->version = http_version; \
    for(; i < arr_size; i++){ \
        c_utils_response_set_field(response, fields[i].field, fields[i].value); \
    } \
} while(0);

/**
 * A helper-macro used for convenience which allows you to declare key-value pairs in the guise of an anonymous struct declaration, I.E
 * C_UTILS_REQUEST_WRITE(res, 200, C_UTILS_HTTP_GET,, "/", NULL);
 * @param request Request.
 * @param http_status The HTTP status.
 * @param http_method The HTTP method.
 * @param file_path The file path.
 * @param ... Variadic arguments of field types, cannot be empty, but can be NULL.
 */
#define C_UTILS_REQUEST_WRITE(request, http_method, http_version, file_path, ...) do { \
    struct c_utils_field *fields = (struct c_utils_field[]){ __VA_ARGS__ }; \
    size_t arr_size = sizeof((struct c_utils_field[]){ __VA_ARGS__ }) / sizeof(struct c_utils_field); \
    int i = 0; \
    request->method = http_method; \
    request->verison = http_version; \
    request->path = file_path; \
    for(; i < arr_size; i++){ \
        c_utils_request_set_field(request, fields[i].field, fields[i].value); \
    } \
} while(0);

/**
 * Sets the field in the mapped header.
 * @param res Response.
 * @param field Field.
 * @param value Value.
 * @return If Successful.
 */
bool c_utils_response_set_field(struct c_utils_response *res, char *field, char *value);

/**
 * Sets the field in the mapped header.
 * @param req Request.
 * @param field Field.
 * @param value Value.
 * @return If Successful.
 */
bool c_utils_request_set_field(struct c_utils_request *req, char *field, char *value);

/**
 * Removes the field in the mapped header.
 * @param res Response.
 * @param field Field.
 * @return If Successful.
 */
bool c_utils_response_remove_field(struct c_utils_response *res, const char *field);

/**
 * Removes the field in the mapped header.
 * @param req Request.
 * @param field Field.
 * @return If Successful.
 */
bool c_utils_request_remove_field(struct c_utils_request *req, const char *field);

/**
 * Obtains the field from the mapped header.
 * @param res Response.
 * @param field Field.
 * @return The value, NULL if not exist.
 */
char *c_utils_response_get_field(struct c_utils_response *res, const char *field);

/**
 * Obtains the field from the mapped header.
 * @param req Request.
 * @param field Field.
 * @return The value, NULL if not exist. 
 */
char *c_utils_request_get_field(struct c_utils_request *req, const char *field);

#endif /* end NET_UTILS_HTTP_H */
