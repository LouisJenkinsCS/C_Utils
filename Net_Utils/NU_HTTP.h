#ifndef NET_UTILS_HTTP_H
#define NET_UTILS_HTTP_H

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
    NU_HTTP_Version_e ver;
    /// The HTTP status.
    unsigned int stat;
} NU_Response_t;

typedef struct {
    /// Contains the fields parsed from the header in a hash table.
    DS_Hash_Map_t *header;
    /// The HTTP method.
    NU_HTTP_Method_e meth;
    /// The request file path.
    char file_path[NU_HTTP_FILE_PATH_LEN + 1];
    /// The HTTP version.
    NU_HTTP_Version_e ver;
} NU_Request_t;

// TODO: Make Field Values case-insensitive!

NU_Response_t *NU_Response_create(void);

NU_Request_t *NU_Request_create(void);

/*
    NOTE: When parsing from header, make sure to NOT read past the header_size. Try to make sure header is of appropriate size.
    This will append what it can to the mapped header, and return what it cannot. I.E After end of header, or incomplete portions.
    header_size will be the size of what is left (I.E what is invalid or not part of the header). Response should be cleared before passing it!
    TODO: Unit Test this with a purposely invalid header, and with an incomplete header, and without a header.
*/
char *NU_Response_append_header(NU_Response_t *res, const char *header, size_t *header_size);

char *NU_Request_append_header(NU_Request_t *req, const char *header, size_t *header_size);

/*
    Clears the response header of all fields and attributes.
*/
bool NU_Response_clear(NU_Response_t *res);

bool NU_Request_clear(NU_Request_t *req);

/*
    Returns the null-terminated string of the header.
*/
char *NU_Response_to_string(NU_Response_t *res);

char *NU_Request_to_string(NU_Request_t *req);

bool NU_Response_set_field(NU_Response_t *res, char *field, char *values);

bool NU_Request_set_field(NU_Request_t *req, char *field, char *values);

bool NU_Response_remove_field(NU_Response_t *res, const char *field);

bool NU_Request_remove_field(NU_Request_t *req, const char *field);

char *NU_Response_get_field(NU_Response_t *res, const char *field);

char *NU_Request_get_field(NU_Request_t *req, const char *field);

NU_HTTP_Version_e NU_Response_get_version(NU_Response_t *res);

NU_HTTP_Version_e NU_Request_get_version(NU_Request_t *req);

bool NU_Response_set_version(NU_Response_t *res, NU_HTTP_Version_e version);

bool NU_Request_set_version(NU_Request_t *req, NU_HTTP_Version_e version);

unsigned int NU_Response_get_status(NU_Response_t *res);

bool NU_Response_set_status(NU_Response_t *res, unsigned int status);

NU_HTTP_Method_e NU_Request_get_method(NU_Request_t *req);

bool NU_Request_set_method(NU_Request_t *req, NU_HTTP_Method_e method);

#endif /* end NET_UTILS_HTTP_H */
