#ifndef NET_UTILS_HTTP_H
#define NET_UTILS_HTTP_H

#ifdef C_UTILS_USE_STD_POSIX
#define response_t NU_Response_t
#define request_t NU_Request_t
#define request_create(...) NU_Request_create(__VA_ARGS__)
#define response_create(...) NU_Response_create(__VA_ARGS__)
#define request_clear(...) NU_Request_clear(__VA_ARGS__)
#define response_clear(...) NU_Response_clear(__VA_ARGS__)
#define request_set_field(...) NU_Request_set_field(__VA_ARGS__)
#define response_set_field(...) NU_Response_set_field(__VA_ARGS__)
#define request_get_field(...) NU_Request_get_field(__VA_ARGS__)
#define response_get_field(...) NU_Response_get_field(__VA_ARGS__)
#define request_append(...)
#define response_append(...)
#define request_to_string(...) NU_Request_to_string(__VA_ARGS__)
#define response_to_string(...) NU_Response_to_string(__VA_ARGS__)
#define REQUEST_WRITE(...) NU_REQUEST_WRITE(__VA_ARGS__)
#define RESPONSE_WRITE(...) NU_RESPONSE_WRITE(__VA_ARGS__)
#endif

#ifdef NU_HTTP_FILE_PATH_MAX_LEN
#define NU_HTTP_FILE_PATH_LEN NU_HTTP_FILE_PATH_MAX_LEN
#else
#define NU_HTTP_FILE_PATH_LEN 128
#endif

#ifdef NU_HTTP_HEADER_FIELD_MAX_LEN
#define NU_HTTP_HEADER_FIELD_LEN NU_HTTP_HEADER_FIELD_MAX_LEN
#else
#define NU_HTTP_HEADER_FIELD_LEN 128
#endif

#ifdef NU_HTTP_HEADER_VALUE_MAX_LEN
#define NU_HTTP_HEADER_VALUE_LEN NU_HTTP_HEADER_VALUE_MAX_LEN
#else
#define NU_HTTP_HEADER_VALUE_LEN 1024
#endif

#ifdef NU_HTTP_HEADER_MAX_LEN
#define NU_HTTP_HEADER_LEN NU_HTTP_HEADER_MAX_LEN
#else
#define NU_HTTP_HEADER_LEN 4096
#endif

#include <NU_Helper.h>
#include <NU_Connection.h>
#include <DS_Hash_Map.h>

typedef enum {
    /// If the method is unitialized
    NU_HTTP_NO_METHOD,
	NU_HTTP_GET,
	NU_HTTP_POST,
	NU_HTTP_HEAD,
	NU_HTTP_DELETE,
	NU_HTTP_TRACE,
	NU_HTTP_CONNECT
} NU_HTTP_Method_e;

typedef enum {
    /// If the version is uninitialized.
    NU_HTTP_NO_VER,
    NU_HTTP_VER_1_0,
    NU_HTTP_VER_1_1,
    NU_HTTP_VER_1_X
} NU_HTTP_Version_e;

typedef struct {
    /// Contains the fields parsed from the header in a hash table.
    DS_Hash_Map_t *header;
    /// The HTTP version.
    NU_HTTP_Version_e version;
    /// The HTTP status.
    unsigned int status;
} NU_Response_t;

typedef struct {
    /// Contains the fields parsed from the header in a hash table.
    DS_Hash_Map_t *header;
    /// The HTTP method.
    NU_HTTP_Method_e method;
    /// The request file path.
    char *path;
    /// The HTTP version.
    NU_HTTP_Version_e version;
} NU_Request_t;

typedef struct {
    char *field;
    char *value;
} NU_Field_t;

// TODO: Make Field Values case-insensitive!

/**
 * Creates a NU_Response_t instance.
 * @return NU_Response_t instance.
 */
NU_Response_t *NU_Response_create(void);

/**
 * Creates a NU_Request_t instance.
 * @return NU_Response_t instance.
 */
NU_Request_t *NU_Request_create(void);

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
char *NU_Response_append(NU_Response_t *res, const char *header, size_t *header_size);

/**
 * Appends and parses the header to the request. The header_size must intially contain the actual size of
 * the header, and will return the offset of which it stopped reading as well as the offset pointer.
 * @param req Instance of request.
 * @param header Header to parse and append.
 * @param header_size Used to obtain size of header, and return the size of the header.
 * @return What could not be read, meaning the beginning of the null terminator on success, otherwise start of character could not parse.
 */
char *NU_Request_append(NU_Request_t *req, const char *header, size_t *header_size);

/**
 * Clears the header of mapped values.
 * @param res Response.
 * @return If Successful.
 */
bool NU_Response_clear(NU_Response_t *res);

/**
 * Clears the header of mapped values.
 * @param req Request.
 * @return If Successful.
 */
bool NU_Request_clear(NU_Request_t *req);

/**
 * Returns the string representation of the parsed/generated header.
 * @param res Response.
 * @return If Successful.
 */
char *NU_Response_to_string(NU_Response_t *res);

/**
 * Returns the string representation of the parsed/generated header.
 * @param req Request.
 * @return If Successful.
 */
char *NU_Request_to_string(NU_Request_t *req);

/**
 * A helper-macro used for convenience which allows you to declare key-value pairs in the guise of an anonymous struct declaration, I.E
 * NU_RESPONSE_WRITE(res, 200, NU_HTTP_VER_1_0, { "Content-Length", "100" }, { "Content-Type", "text/html" });
 * @param response Response.
 * @param http_status The HTTP status.
 * @param http_version The HTTP version.
 * @param ... Variadic arguments of field types, cannot be empty, but can be NULL.
 */
#define NU_RESPONSE_WRITE(response, http_status, http_version, ...) do { \
    NU_Field_t *fields = (NU_Field_t[]){ __VA_ARGS__ }; \
    size_t arr_size = sizeof((NU_Field_t[]){ __VA_ARGS__ }) / sizeof(NU_Field_t); \
    int i = 0; \
    response->status = http_status; \
    response->version = http_version; \
    for(; i < arr_size; i++){ \
        NU_Response_set_field(response, fields[i].field, fields[i].value); \
    } \
} while(0);

/**
 * A helper-macro used for convenience which allows you to declare key-value pairs in the guise of an anonymous struct declaration, I.E
 * NU_REQUEST_WRITE(res, 200, NU_HTTP_GET,, "/", NULL);
 * @param request Request.
 * @param http_status The HTTP status.
 * @param http_method The HTTP method.
 * @param file_path The file path.
 * @param ... Variadic arguments of field types, cannot be empty, but can be NULL.
 */
#define NU_REQUEST_WRITE(request, http_method, http_version, file_path, ...) do { \
    NU_Field_t *fields = (NU_Field_t[]){ __VA_ARGS__ }; \
    size_t arr_size = sizeof((NU_Field_t[]){ __VA_ARGS__ }) / sizeof(NU_Field_t); \
    int i = 0; \
    request->method = http_method; \
    request->verison = http_version; \
    request->path = file_path; \
    for(; i < arr_size; i++){ \
        NU_Request_set_field(request, fields[i].field, fields[i].value); \
    } \
} while(0);

/**
 * Sets the field in the mapped header.
 * @param res Response.
 * @param field Field.
 * @param value Value.
 * @return If Successful.
 */
bool NU_Response_set_field(NU_Response_t *res, char *field, char *value);

/**
 * Sets the field in the mapped header.
 * @param req Request.
 * @param field Field.
 * @param value Value.
 * @return If Successful.
 */
bool NU_Request_set_field(NU_Request_t *req, char *field, char *value);

/**
 * Removes the field in the mapped header.
 * @param res Response.
 * @param field Field.
 * @return If Successful.
 */
bool NU_Response_remove_field(NU_Response_t *res, const char *field);

/**
 * Removes the field in the mapped header.
 * @param req Request.
 * @param field Field.
 * @return If Successful.
 */
bool NU_Request_remove_field(NU_Request_t *req, const char *field);

/**
 * Obtains the field from the mapped header.
 * @param res Response.
 * @param field Field.
 * @return The value, NULL if not exist.
 */
char *NU_Response_get_field(NU_Response_t *res, const char *field);

/**
 * Obtains the field from the mapped header.
 * @param req Request.
 * @param field Field.
 * @return The value, NULL if not exist. 
 */
char *NU_Request_get_field(NU_Request_t *req, const char *field);

#endif /* end NET_UTILS_HTTP_H */
