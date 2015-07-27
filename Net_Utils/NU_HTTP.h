#ifndef NET_UTILS_HTTP_H
#define NET_UTILS_HTTP_H

#include <NU_Helper.h>
#include <NU_Connection.h>
#include <DS_Hash_Map.h>

const char *NU_HTTP_Status_Codes[] = {
	[100] = "100 Continue",
    [101] = "101 Switching Protocols",

    // Success 2xx
    [200] = "200 OK",
    [201] = "201 Created",
    [202] = "202 Accepted",
    [203] = "203 Non-Authoritative Information",
    [204] = "204 No Content",
    [205] = "205 Reset Content",
    [206] = "206 Partial Content",

    // Redirection 3xx
    [300] = "300 Multiple Choices",
    [301] = "301 Moved Permanently",
    [302] = "302 Found",
    [303] = "303 See Other",
    [304] = "304 Not Modified",
    [305] = "305 Use Proxy",
    [307] = "307 Temporary Redirect",

    // Client Error 4xx
    [400] = "400 Bad Request",
    [401] = "401 Unauthorized",
    [402] = "402 Payment Required",
    [403] = "403 Forbidden",
    [404] = "404 Not Found",
    [405] = "405 Method Not Allowed",
    [406] = "406 Not Acceptable",
    [407] = "407 Proxy Authentication Required",
    [408] = "408 Request Timeout",
    [409] = "409 Conflict",
    [410] = "410 Gone",
    [411] = "411 Length Required",
    [412] = "412 Precondition Failed",
    [413] = "413 Request Entity Too Large",
    [414] = "414 Request-URI Too Long",
    [415] = "415 Unsupported Media Type",
    [416] = "416 Requested Range Not Satisfiable",
    [417] = "417 Expectation Failed",

    // Server Error 5xx
    [500] = "500 Internal Server Error",
    [501] = "501 Not Implemented",
    [502] = "502 Bad Gateway",
    [503] = "503 Service Unavailable",
    [504] = "504 Gateway Timeout",
    [505] = "505 HTTP Version Not Supported",
    [509] = "509 Bandwidth Limit Exceeded"
};

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

NU_Header_t *NU_Header_from(const char *header_str);

char *NU_Header_to_string(NU_Header_t *header);

#endif /* end NET_UTILS_HTTP_H */
