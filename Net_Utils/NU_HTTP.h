#ifndef NET_UTILS_HTTP_H
#define NET_UTILS_HTTP_H

#include <NU_Helper.h>

const char *NU_HTTP_Status_Codes[] = {
	[100] = 'Continue',
    [101] = 'Switching Protocols',

    // Success [2]xx
    [200] = 'OK',
    [201] = 'Created',
    [202] = 'Accepted',
    [203] = 'Non-Authoritative Information',
    [204] = 'No Content',
    [205] = 'Reset Content',
    [206] = 'Partial Content',

    // Redirection [3]xx
    [300] = 'Multiple Choices',
    [301] = 'Moved Permanently',
    [302] = 'Found',
    [303] = 'See Other',
    [304] = 'Not Modified',
    [305] = 'Use Proxy',
    [307] = 'Temporary Redirect',

    // Client Error 4xx
    [400] = 'Bad Request',
    [401] = 'Unauthorized',
    [402] = 'Payment Required',
    [403] = 'Forbidden',
    [404] = 'Not Found',
    [405] = 'Method Not Allowed',
    [406] = 'Not Acceptable',
    [407] = 'Proxy Authentication Required',
    [408] = 'Request Timeout',
    [409] = 'Conflict',
    [410] = 'Gone',
    [411] = 'Length Required',
    [412] = 'Precondition Failed',
    [413] = 'Request Entity Too Large',
    [414] = 'Request-URI Too Long',
    [415] = 'Unsupported Media Type',
    [416] = 'Requested Range Not Satisfiable',
    [417] = 'Expectation Failed',

    // Server Error 5xx
    [500] = 'Internal Server Error',
    [501] = 'Not Implemented',
    [502] = 'Bad Gateway',
    [503] = 'Service Unavailable',
    [504] = 'Gateway Timeout',
    [505] = 'HTTP Version Not Supported',
    [509] = 'Bandwidth Limit Exceeded'
};

typedef enum {
	NU_HTTP_GET,
	NU_HTTP_POST,
	NU_HTTP_HEAD,
	NU_HTTP_DELETE,
	NU_HTTP_TRACE,
	NU_HTTP_CONNECT
} NU_HTTP_Method_e;

// _f suffix denotes "field" to avoid any conflicts with function names.
typedef struct {
   char *accept_f;
   char *accept_charset_f;
   char *accept_encoding_f;
   char *accept_languages_f;
   char *accept_datetime_f;
   char *authorization_f;
   // TODO: Continue from here!
} NU_HTTP_Header_t;

typedef struct {
	NU_HTTP_version;
} NU_HTTP_Response_t;

typedef struct {

} NU_HTTP_Request_t;

#endif /* end NET_UTILS_HTTP_H */
