#include <NU_HTTP.h>
#include <ctype.h>

/// 31 is close enough to the amount of HTTP statuses, but is also a prime number.
static const int bucket_size = 31;

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Net_Utils/Logs/NU_HTTP.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

static const char *NU_HTTP_Status_Codes[] = {
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

static void parse_http_field(DS_Hash_Map_t *mapped_fields, const char *line){
	MU_DEBUG("%s", line);
	const char *delimiter = ": ";
	const int delim_size = strlen(delimiter);
	char field[NU_HTTP_HEADER_FIELD_LEN + 1], value[NU_HTTP_HEADER_VALUE_LEN + 1];
	char *offset_str = strstr(line, delimiter);
	if(!offset_str){
		MU_LOG_WARNING(logger, "No delimiter offset for line: '%s'!", line);
		return;
	}
	/*
		A bit of math required here. We must find the length of the field to read into field buffer.
		offset_str is just (line + x) for some offset 'x', so it evaluates to the following:
		(line + x) - line = x. Hence field_len is the offset, from 0 to x.
	*/
	int field_len = offset_str - line;
	offset_str += delim_size;
	MU_DEBUG("Field: '%.*s'\nValue: '%s'", field_len, line, offset_str);
	snprintf(field, NU_HTTP_HEADER_FIELD_LEN, "%.*s", field_len, line);
	snprintf(value, NU_HTTP_HEADER_VALUE_LEN, "%s", offset_str);
	bool was_added = DS_Hash_Map_add(mapped_fields, field, strdup(value));
	if(!was_added){
		MU_LOG_WARNING(logger, "DS_Hash_Map_add: 'Was unable to add key-value pair ('%s': '%s')!'");
	}
}

static void parse_http_method(NU_Request_t *req, const char *line){
	MU_DEBUG("%s", line);
	if(strncmp(line, "GET", 3) == 0){
		req->method = NU_HTTP_GET;
	} else if(strncmp(line, "POST", 4) == 0){
		req->method = NU_HTTP_POST;
	} else if(strncmp(line, "HEAD", 4) == 0){
		req->method = NU_HTTP_HEAD;
	} else if(strncmp(line, "DELETE", 6) == 0){
		req->method = NU_HTTP_DELETE;
	} else if(strncmp(line, "TRACE", 5) == 0){
		req->method = NU_HTTP_TRACE;
	} else if(strncmp(line, "CONNECT", 7) == 0){
		req->method = NU_HTTP_CONNECT;
	} else {
		MU_LOG_WARNING(logger, "Bad HTTP method!'%s'", line);
	}
}

static void parse_http_path(NU_Request_t *req, const char *line){
	MU_DEBUG("%s", line);
	if(strlen(line) == 1){
		sprintf(req->file_path, "/index.html");
	}
	snprintf(req->file_path, NU_HTTP_FILE_PATH_LEN, "%s", line);
}

static void parse_http_status(NU_Response_t *res, const char *line){
	MU_DEBUG("%s", line);
	int status = strtol(line, NULL, 10);
	if(!status){
		MU_LOG_WARNING(logger, "Bad HTTP status!");
		return;
	}
	res->status = status;
}

static void parse_http_version(NU_HTTP_Version_e *version, const char *line){
	MU_DEBUG("%s", line);
	const int protocol_len = 8;
	if(strncmp(line, "HTTP/1.1", protocol_len) == 0){
		*version = NU_HTTP_VER_1_1;
	} else if(strncmp(line, "HTTP/1.0", protocol_len) == 0){
		*version = NU_HTTP_VER_1_0;
	} else if(strncmp(line, "HTTP/1.x", protocol_len) == 0){
		*version = NU_HTTP_VER_1_X;
	} else {
		MU_LOG_WARNING(logger, "Bad HTTP version!");
		*version = NU_HTTP_NO_VER;
	}
}

static void parse_http_response(NU_Response_t *header, char *header_str){
	MU_DEBUG("Header: \n%s", header_str);
	char *first_line;
	char *rest_of_lines;
	char *line = strtok_r(header_str, "\r\n", &rest_of_lines);
	if(!line){
		MU_LOG_WARNING(logger, "No header field found!");
		return;
	}
	/// For the first line, tokenate out the method, path (if applicable) and http version.
	line = strtok_r(line, " ", &first_line);
	do {
		if(!line){
			MU_LOG_WARNING(logger, "Invalid first line of header!'%s'", line);
			return;
		}
		if(strncmp(line, "HTTP", 4) == 0){
			parse_http_version(&header->version, line);
		} else if (isdigit((unsigned char)*line)){
			parse_http_status(header, line);
		} else {
			MU_LOG_WARNING(logger, "Invalid header format!'%s'", line);
		}
	} while((line = strtok_r(NULL, " ", &first_line)));
	while((line = strtok_r(NULL, "\r\n", &rest_of_lines))){
		parse_http_field(header->header, line);
	}
}

static size_t parse_http_request(NU_Request_t *req, char *header_str){
	MU_DEBUG("Header: \n%s", header_str);
	size_t header_size = 0;
	char *first_line;
	char *rest_of_lines;
	char *line = strtok_r(header_str, "\r\n", &rest_of_lines);
	if(!line){
		MU_LOG_WARNING(logger, "No header field found!'%s'", line);
		return 0;
	}
	// strlen(line) + strlen("\r\n");
	header_size += strlen(line) + 2;
	/// For the first line, tokenate out the method, path (if applicable) and http version.
	line = strtok_r(line, " ", &first_line);
	do {
		if(!line){
			MU_LOG_WARNING(logger, "Invalid first line of header!'%s'", line);
			return 0;
		}
		if(strncmp(line, "HTTP", 4) == 0){
			parse_http_version(&req->version, line);
		} else if(*line == '/'){
			parse_http_path(req, line);
		} else parse_http_method(req, line);
	} while((line = strtok_r(NULL, " ", &first_line)));
	while((line = strtok_r(NULL, "\r\n", &rest_of_lines))){
		parse_http_field(req->header, line);
		// strlen(line) + strlen("\r\n");
		header_size += strlen(line) + 2;
	}
	// strlen(line) + strlen("\r\n");
	return header_size += 2;
}

NU_Response_t *NU_Response_create(void){
	NU_Response_t *res = calloc(1, sizeof(NU_Response_t));
	if(!res){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	res->header = DS_Hash_Map_create(bucket_size, true);
	if(!res->header){
		MU_LOG_ERROR(logger, "DS_Hash_Map_create: 'Was unable to create Hash Table!'");
		free(res);
		return NULL;
	}
	return res;
}

NU_Request_t *NU_Request_create(void){
	NU_Request_t *req = calloc(1, sizeof(NU_Response_t));
	if(!req){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	req->header = DS_Hash_Map_create(bucket_size, true);
	if(!req->header){
		MU_LOG_ERROR(logger, "DS_Hash_Map_create: 'Was unable to create Hash Table!'");
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
char *NU_Response_append_header(NU_Response_t *res, const char *header, size_t *header_size);

char *NU_Request_append_header(NU_Request_t *req, const char *header, size_t *header_size){
	char header_cpy[*header_size + 1];
	snprintf(header_cpy, *header_size + 1, "%s", header);
	size_t header_read = parse_http_request(req, header_cpy);
	MU_DEBUG("Read %zu of %zu bytes of header; New header size = %zu!", header_read, *header_size, *header_size - header_read);
	*header_size -= header_read;
	return (char *)header + header_read;
}

/*
    Clears the response header of all fields and attributes.
*/
bool NU_Response_clear(NU_Response_t *res);

bool NU_Request_clear(NU_Request_t *req);

/*
    Returns the null-terminated string of the header.
*/
char *NU_Response_to_string(NU_Response_t *res);

char *NU_Request_to_string(NU_Request_t *req){
	char *buf = calloc(1, NU_HTTP_HEADER_LEN + 1);
	size_t size, i = 0, size_left = NU_HTTP_HEADER_LEN + 1;
	char **arr = DS_Hash_Map_key_value_to_string(req->header, NULL, ": ", NULL, &size, NULL);
	for(; i < size; i++){
		// Length of the mapped value plus 2 bytes for carriage return.
		size_t str_len = strlen(arr[i]) + 2;
		if(size_left < str_len) break;
		snprintf(buf, size_left, "%s%s\r\n", buf, arr[i]);
		free(arr[i]);
		size_left -= str_len;
	}
	sprintf(buf, "%s\r\n\r\n", buf);
	free(arr);
	char *tmp_buf = realloc(buf, strlen(buf) + 1);
	if(!tmp_buf){
		MU_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		return buf;
	}
	return tmp_buf;
}

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
