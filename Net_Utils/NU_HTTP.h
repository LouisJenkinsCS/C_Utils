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

// _f suffix denotes "field" to avoid any conflicts with function names.
// Note to self: Use a hash table for each of these! Would be rather efficient in the long run, with O(1) processing time.
typedef struct {
    /// Contains the fields parsed from the header in a hash table.
   DS_Hash_Map_t *mapped_fields;
   /// The HTTP version.
   NU_HTTP_Version_e version;
   /// The HTTP method.
   NU_HTTP_Method_e method;
   /// The HTTP path.
   char file_path[256];
   /// The HTTP status.
   unsigned int status;
   /// Determines whether or not this is in use.
   bool in_use;
   // TODO: Continue from here!
} NU_Header_t;

typedef struct {

} NU_Response_t;

typedef struct {

} NU_Request_t;

NU_Header_t *NU_Header_create(void);

NU_Header_t *NU_Header_from(const char *header_str, int header_size);

char *NU_Header_to_string(NU_Header_t *header);

#endif /* end NET_UTILS_HTTP_H */
