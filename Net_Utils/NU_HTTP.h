#ifndef NET_UTILS_HTTP_H
#define NET_UTILS_HTTP_H

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
    DS_Hash_Map_t *mapped_fields;
    /// The HTTP version.
    NU_HTTP_Version_e version;
    /// The HTTP status.
    unsigned int status;
} NU_Response_t;

typedef struct {
    /// Contains the fields parsed from the header in a hash table.
    DS_Hash_Map_t *mapped_fields;
    /// The HTTP method.
    NU_HTTP_Method_e method;
    /// The request file path.
    char file_path[256];
    /// The HTTP version.
    NU_HTTP_Version_e version;
} NU_Request_t;

// TODO: Make Field Values case-insensitive!

NU_Response_t *NU_Response_create(const char *header, int header_size);

NU_Request_t *NU_Request_create(const char *header, int header_size);

bool NU_Response_append_header(NU_Response_t *res, const char *header);

bool NU_Request_append_header(NU_Request_t *req, const char *header);

/// Returns the portion after the http header.
char *NU_Response_to_string(NU_Response_t *res, char *buffer, size_t *buf_size);

char *NU_Request_to_string(NU_Request_t *req, char *buffer, size_t *buf_size);

bool NU_Response_set_field(NU_Response_t *res, const char *field, const char *values);

bool NU_Request_set_field(NU_Request_t *req, const char *field, const char *values);

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

NU_HTTP_Method_e NU_Response_get_method(NU_Response_t *res);

bool NU_Response_set_method(NU_Response_t *res, NU_HTTP_Method_e method);

#endif /* end NET_UTILS_HTTP_H */
