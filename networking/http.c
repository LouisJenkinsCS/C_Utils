#include "networking/http.h"
#include <ctype.h>

/// 31 is close enough to the amount of HTTP statuses, but is also a prime number.
static const int bucket_size = 31;

static struct c_utils_logger *logger = NULL;

LOGGER_AUTO_CREATE(logger, "./networking/logs/http.log", "w", C_UTILS_LOG_LEVEL_ALL);

static const char *C_UTILS_HTTP_Status_Codes[] = {
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

static void parse_http_field(struct c_utils_map *mapped_fields, const char *line) {
	C_UTILS_LOG_TRACE(logger, "%s", line);

	const char *delimiter = ": ";
	const int delim_size = strlen(delimiter);
	char field[C_UTILS_HTTP_HEADER_FIELD_LEN + 1], value[C_UTILS_HTTP_HEADER_VALUE_LEN + 1];

	char *offset_str = strstr(line, delimiter);
	if (!offset_str) {
		C_UTILS_LOG_WARNING(logger, "No delimiter offset for line: '%s'!", line);
		return;
	}

	/*
		A bit of math required here. We must find the length of the field to read into field buffer.
		offset_str is just (line + x) for some offset 'x', so it evaluates to the following:
		(line + x) - line = x. Hence field_len is the offset, from 0 to x.
	*/
	int field_len = offset_str - line;
	offset_str += delim_size;
	C_UTILS_LOG_TRACE(logger, "Field: '%.*s'\nValue: '%s'", field_len, line, offset_str);
	
	/*
		We read up to a maximum of the configured max header field and value lengths respectively,
		using *printf precision. Hence, we read MIN(field_len , HTTP_HEADER_FIELD_LEN).  
	*/
	snprintf(field, C_UTILS_HTTP_HEADER_FIELD_LEN, "%.*s", field_len, line);
	snprintf(value, C_UTILS_HTTP_HEADER_VALUE_LEN, "%s", offset_str);

	bool was_added = c_utils_map_add(mapped_fields, field, strdup(value));
	if (!was_added)  
		C_UTILS_LOG_WARNING(logger, "c_utils_map_add: 'Was unable to add key-value pair ('%s': '%s')!'");
	
}

static void parse_http_method(struct c_utils_request *req, const char *line) {
	C_UTILS_LOG_TRACE(logger, "%s", line);

	if (strncmp(line, "GET", 3) == 0)  
		req->method = C_UTILS_HTTP_GET;
	 else if (strncmp(line, "POST", 4) == 0)  
		req->method = C_UTILS_HTTP_POST;
	 else if (strncmp(line, "HEAD", 4) == 0)  
		req->method = C_UTILS_HTTP_HEAD;
	 else if (strncmp(line, "DELETE", 6) == 0)  
		req->method = C_UTILS_HTTP_DELETE;
	 else if (strncmp(line, "TRACE", 5) == 0)  
		req->method = C_UTILS_HTTP_TRACE;
	 else if (strncmp(line, "CONNECT", 7) == 0)  
		req->method = C_UTILS_HTTP_CONNECT;
	 else   
		C_UTILS_LOG_WARNING(logger, "Bad HTTP method!'%s'", line);
	
}

static void parse_http_path(struct c_utils_request *req, const char *line) {
	C_UTILS_LOG_TRACE(logger, "%s", line);

	if (strlen(line) == 1) {
		req->path = strdup("/index.html");
		return;
	}

	req->path = strdup(line);
}

static void parse_http_status(struct c_utils_response *res, const char *line) {
	C_UTILS_LOG_TRACE(logger, "%s", line);

	int status = strtol(line, NULL, 10);
	if (!status) {
		C_UTILS_LOG_WARNING(logger, "Bad HTTP status!");
		return;
	}

	res->status = status;
}

static void parse_http_version(C_UTILS_HTTP_Version_e *version, const char *line) {
	C_UTILS_LOG_TRACE(logger, "%s", line);
	
	const int protocol_len = 8;
	if (strncasecmp(line, "HTTP/1.1", protocol_len) == 0)  
		*version = C_UTILS_HTTP_VER_1_1;
	 else if (strncasecmp(line, "HTTP/1.0", protocol_len) == 0)  
		*version = C_UTILS_HTTP_VER_1_0;
	 else if (strncasecmp(line, "HTTP/1.X", protocol_len) == 0)  
		*version = C_UTILS_HTTP_VER_1_X;
	 else {
		C_UTILS_LOG_WARNING(logger, "Bad HTTP version!");
		*version = C_UTILS_HTTP_NO_VER;
	}
}

static char *http_method_to_string(enum c_utils_http_method method) {
	switch(method) {
		case C_UTILS_HTTP_GET: return "GET";
		case C_UTILS_HTTP_POST: return "POST";
		case C_UTILS_HTTP_CONNECT: return "CONNECT";
		case C_UTILS_HTTP_TRACE: return "TRACE";
		case C_UTILS_HTTP_DELETE: return "DELETE";
		case C_UTILS_HTTP_HEAD: return "HEAD";
		default: return NULL;
	}
}

static char *http_version_to_string(enum c_utils_http_version version) {
	switch(version) {
		case C_UTILS_HTTP_VER_1_0: return "HTTP/1.0";
		case C_UTILS_HTTP_VER_1_1: return "HTTP/1.1";
		case C_UTILS_HTTP_VER_1_X: return "HTTP/1.X";
		default: return NULL;
	}
}

static size_t parse_http_response(struct c_utils_response *header, char *header_str) {
	C_UTILS_LOG_TRACE(logger, "Header: \n%s", header_str);
	
	size_t header_size = 0;
	char *first_line;
	char *rest_of_lines;
	
	char *line = strtok_r(header_str, "\r\n", &rest_of_lines);
	if (!line) {
		C_UTILS_LOG_WARNING(logger, "No header field found!");
		return 0;
	}
	
	// strlen(line) + strlen("\r\n");
	header_size += strlen(line) + 2;
	
	/// For the first line, tokenate out the method, path (if applicable) and http version.
	line = strtok_r(line, " ", &first_line);
	do {
		if (!line) {
			C_UTILS_LOG_WARNING(logger, "Invalid first line of header!'%s'", line);
			return 0;
		}
		
		if (strncmp(line, "HTTP", 4) == 0)  
			parse_http_version(&header->version, line);
		 else if (isdigit((unsigned char)*line))  
			parse_http_status(header, line);
		 else   
			C_UTILS_LOG_WARNING(logger, "Invalid header format!'%s'", line);
		
	} while ((line = strtok_r(NULL, " ", &first_line)));
	
	while ((line = strtok_r(NULL, "\r\n", &rest_of_lines))) {
		// strlen(line) + strlen("\r\n");
		header_size += strlen(line) + 2;
		parse_http_field(header->header, line);
	}
	
	// header_size + strlen("\r\n")
	return header_size += 2;
}

static size_t parse_http_request(struct c_utils_request *req, char *header_str) {
	C_UTILS_LOG_TRACE(logger, "Header: \n%s", header_str);
	
	size_t header_size = 0;
	char *first_line;
	char *rest_of_lines;
	
	char *line = strtok_r(header_str, "\r\n", &rest_of_lines);
	if (!line) {
		C_UTILS_LOG_WARNING(logger, "No header field found!'%s'", line);
		return 0;
	}
	
	// strlen(line) + strlen("\r\n");
	header_size += strlen(line) + 2;
	
	/// For the first line, tokenate out the method, path (if applicable) and http version.
	line = strtok_r(line, " ", &first_line);
	do {
		if (!line) {
			C_UTILS_LOG_WARNING(logger, "Invalid first line of header!'%s'", line);
			return 0;
		}
		
		if (strncmp(line, "HTTP", 4) == 0)  
			parse_http_version(&req->version, line);
		 else if (*line == '/')  
			parse_http_path(req, line);
		 else parse_http_method(req, line);
	} while ((line = strtok_r(NULL, " ", &first_line)));
	
	while ((line = strtok_r(NULL, "\r\n", &rest_of_lines))) {
		parse_http_field(req->header, line);
		// strlen(line) + strlen("\r\n");
		header_size += strlen(line) + 2;
	}
	
	// header_size + strlen("\r\n")
	return header_size += 2;
}

struct c_utils_response *c_utils_response_create(void) {
	struct c_utils_response *res = calloc(1, sizeof(struct c_utils_response));
	if (!res) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	
	// Synchronized Map
	res->header = c_utils_map_create(bucket_size, true);
	if (!res->header) {
		C_UTILS_LOG_ERROR(logger, "c_utils_map_create: 'Was unable to create Hash Table!'");
		free(res);
		return NULL;
	}
	
	return res;
}

struct c_utils_request *c_utils_request_create(void) {
	struct c_utils_request *req = calloc(1, sizeof(struct c_utils_request));
	if (!req) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	
	// Synchronized Map
	req->header = c_utils_map_create(bucket_size, true);
	if (!req->header) {
		C_UTILS_LOG_ERROR(logger, "c_utils_map_create: 'Was unable to create Hash Table!'");
		free(req);
		return NULL;
	}
	
	return req;
}

/*
    NOTE: When parsing from header, make sure to NOT read past the header_size. Try to make sure header is of appropriate size.
    This will append what it can to the mapped header, and return what it cannot. I.E After end of header, or incomplete portions.
    header_size will be the size of what is left (I.E what is invalid or not part of the header). Response should be cleared before passing it!
    TODO: Unit Test this with a purposely invalid header, and with an incomplete header, and without a header.
*/
char *c_utils_response_append(struct c_utils_response *res, const char *header, size_t *header_size) {
	char header_cpy[*header_size + 1];

	snprintf(header_cpy, *header_size + 1, "%s", header);
	size_t header_read = parse_http_response(res, header_cpy);
	C_UTILS_LOG_TRACE(logger, "Read %zu of %zu bytes of header; New header size = %zu!", header_read, *header_size, *header_size - header_read);
	*header_size -= header_read;

	return (char *)header + header_read;
}

char *c_utils_request_append(struct c_utils_request *req, const char *header, size_t *header_size) {
	char header_cpy[*header_size + 1];
	
	snprintf(header_cpy, *header_size + 1, "%s", header);
	size_t header_read = parse_http_request(req, header_cpy);
	C_UTILS_LOG_TRACE(logger, "Read %zu of %zu bytes of header; New header size = %zu!", header_read, *header_size, *header_size - header_read);
	*header_size -= header_read;
	
	return (char *)header + header_read;
}

/*
    Clears the response header of all fields and attributes.
*/
bool c_utils_response_clear(struct c_utils_response *res) {
	C_UTILS_ARG_CHECK(logger, false, res);
	
	bool header_cleared = c_utils_map_clear(res->header, free);
	if (!header_cleared) {
		C_UTILS_LOG_ERROR(logger, "c_utils_map_clear: 'Was unable to clear hash map!'");
		return false;
	}
	
	res->version = C_UTILS_HTTP_NO_VER;
	res->status = 0;
	
	return true;

}

bool c_utils_request_clear(struct c_utils_request *req) {
	C_UTILS_ARG_CHECK(logger, false, req);
	
	bool header_cleared = c_utils_map_clear(req->header, free);
	if (!header_cleared) {
		C_UTILS_LOG_ERROR(logger, "c_utils_map_clear: 'Was unable to clear hash map!'");
		return false;
	}
	
	req->version = C_UTILS_HTTP_NO_VER;
	req->method = C_UTILS_HTTP_NO_METHOD;
	
	free(req->path);
	req->path = NULL;

	return true;
}

/*
    Returns the null-terminated string of the header.
*/
char *c_utils_response_to_string(struct c_utils_response *res) {
	C_UTILS_ARG_CHECK(logger, NULL, res);

	char *buf = calloc(1, C_UTILS_HTTP_HEADER_LEN + 1);
	if (!buf) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}

	size_t size, i = 0, size_left = C_UTILS_HTTP_HEADER_LEN + 1;
	
	const char *status = (res->status > 509) ? NULL : C_UTILS_HTTP_Status_Codes[res->status];
	if (!status) {
		C_UTILS_LOG_INFO(logger, "Invalid HTTP Status!");
		goto error;
	}

	char *version = http_version_to_string(res->version);
	if (!version) {
		C_UTILS_LOG_INFO(logger, "Invalid HTTP Version!");
		goto error;
	}

	size_t retval = snprintf(buf, size_left, "%s %s\r\n", version, status);
	size_left -= retval;
	
	char **arr = c_utils_map_key_value_to_string(res->header, NULL, ": ", NULL, &size, NULL);
	for (; i < size; i++) {
		// Length of the mapped value plus 2 bytes for carriage return.
		size_t str_len = strlen(arr[i]) + 2;
		if (size_left < str_len) break;
		
		snprintf(buf, size_left, "%s%s\r\n", buf, arr[i]);
		free(arr[i]);
		size_left -= str_len;
	}
	sprintf(buf, "%s\r\n\r\n", buf);
	free(arr);

	char *tmp_buf = realloc(buf, strlen(buf) + 1);
	if (!tmp_buf) {
		C_UTILS_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		return buf;
	}
	return tmp_buf;

	error:
		if (buf)  
			free(buf);
		
		return NULL;
}

char *c_utils_request_to_string(struct c_utils_request *req) {
	C_UTILS_ARG_CHECK(logger, NULL, req);

	char *buf = calloc(1, C_UTILS_HTTP_HEADER_LEN + 1);
	if (!buf) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	
	size_t size, i = 0, size_left = C_UTILS_HTTP_HEADER_LEN + 1;
	
	char *method = http_method_to_string(req->method);
	if (!method) {
		C_UTILS_LOG_INFO(logger, "Invalid HTTP Method!");
		goto error;
	}
	
	if (!*(req->path)) {
		C_UTILS_LOG_INFO(logger, "Invalid File Path!");
		goto error;
	}

	char *version = http_version_to_string(req->version);
	if (!version) {
		C_UTILS_LOG_INFO(logger, "Invalid HTTP Version!");
		goto error;
	}
	
	size_t retval = snprintf(buf, size_left, "%s %s %s\r\n", method, req->path, version);
	size_left -= retval;
	
	char **arr = c_utils_map_key_value_to_string(req->header, NULL, ": ", NULL, &size, NULL);
	for (; i < size; i++) {
		// Length of the mapped value plus 2 bytes for carriage return.
		size_t str_len = strlen(arr[i]) + 2;
		if (size_left < str_len) break;
		
		snprintf(buf, size_left, "%s%s\r\n", buf, arr[i]);
		free(arr[i]);
		size_left -= str_len;
	}
	sprintf(buf, "%s\r\n\r\n", buf);
	free(arr);
	
	char *tmp_buf = realloc(buf, strlen(buf) + 1);
	if (!tmp_buf) {
		C_UTILS_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		return buf;
	}
	return tmp_buf;

	error:
		if (buf)  
			free(buf);
		
		return NULL;
}

bool c_utils_response_set_field(struct c_utils_response *res, char *field, char *values) {
	C_UTILS_ARG_CHECK(logger, false, res, field, values);

	c_utils_map_remove(res->header, field, free);
	c_utils_map_add(res->header, field, values);
	return true;
}

bool c_utils_request_set_field(struct c_utils_request *req, char *field, char *values) {
	C_UTILS_ARG_CHECK(logger, false, req, field, values);

	c_utils_map_remove(req->header, field, free);
	c_utils_map_add(req->header, field, values);
	return true;
}

bool c_utils_response_remove_field(struct c_utils_response *res, const char *field) {
	C_UTILS_ARG_CHECK(logger, false, res, field);

	c_utils_map_remove(res->header, field, free);
	return true;
}

bool c_utils_request_remove_field(struct c_utils_request *req, const char *field) {
	C_UTILS_ARG_CHECK(logger, false, req, field);

	c_utils_map_remove(req->header, field, free);
	return true;
}

char *c_utils_response_get_field(struct c_utils_response *res, const char *field) {
	C_UTILS_ARG_CHECK(logger, false, res, field);

	char *value = c_utils_map_get(res->header, field);
	return value;
}

char *c_utils_request_get_field(struct c_utils_request *req, const char *field) {
	C_UTILS_ARG_CHECK(logger, false, req, field);

	char *value = c_utils_map_get(req->header, field);
	return value;
}
