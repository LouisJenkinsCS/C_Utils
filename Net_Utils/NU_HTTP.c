#include <NU_HTTP.h>
#include <ctype.h>

/// 31 is close enough to the amount of HTTP statuses, but is also a prime number.
static const int bucket_size = 31;

static MU_Logger_t *logger = NULL;

MU_LOGGER_AUTO_CREATE(logger, "./Net_Utils/Logs/NU_HTTP.log", "w", MU_ALL);

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
	MU_LOG_TRACE(logger, "%s", line);
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
	MU_LOG_TRACE(logger, "Field: '%.*s'\nValue: '%s'", field_len, line, offset_str);
	snprintf(field, NU_HTTP_HEADER_FIELD_LEN, "%.*s", field_len, line);
	snprintf(value, NU_HTTP_HEADER_VALUE_LEN, "%s", offset_str);
	bool was_added = DS_Hash_Map_add(mapped_fields, field, strdup(value));
	if(!was_added){
		MU_LOG_WARNING(logger, "DS_Hash_Map_add: 'Was unable to add key-value pair ('%s': '%s')!'");
	}
}

static void parse_http_method(NU_Request_t *req, const char *line){
	MU_LOG_TRACE(logger, "%s", line);
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
	MU_LOG_TRACE(logger, "%s", line);
	if(strlen(line) == 1){
		req->path = "/index.html";
		return;
	}
	req->path = strdup(line);
}

static void parse_http_status(NU_Response_t *res, const char *line){
	MU_LOG_TRACE(logger, "%s", line);
	int status = strtol(line, NULL, 10);
	if(!status){
		MU_LOG_WARNING(logger, "Bad HTTP status!");
		return;
	}
	res->status = status;
}

static void parse_http_version(NU_HTTP_Version_e *version, const char *line){
	MU_LOG_TRACE(logger, "%s", line);
	const int protocol_len = 8;
	if(strncasecmp(line, "HTTP/1.1", protocol_len) == 0){
		*version = NU_HTTP_VER_1_1;
	} else if(strncasecmp(line, "HTTP/1.0", protocol_len) == 0){
		*version = NU_HTTP_VER_1_0;
	} else if(strncasecmp(line, "HTTP/1.X", protocol_len) == 0){
		*version = NU_HTTP_VER_1_X;
	} else {
		MU_LOG_WARNING(logger, "Bad HTTP version!");
		*version = NU_HTTP_NO_VER;
	}
}

static char *http_method_to_string(NU_HTTP_Method_e method){
	switch(method){
		case NU_HTTP_GET: return "GET";
		case NU_HTTP_POST: return "POST";
		case NU_HTTP_CONNECT: return "CONNECT";
		case NU_HTTP_TRACE: return "TRACE";
		case NU_HTTP_DELETE: return "DELETE";
		case NU_HTTP_HEAD: return "HEAD";
		default: return NULL;
	}
}

static char *http_version_to_string(NU_HTTP_Version_e version){
	switch(version){
		case NU_HTTP_VER_1_0: return "HTTP/1.0";
		case NU_HTTP_VER_1_1: return "HTTP/1.1";
		case NU_HTTP_VER_1_X: return "HTTP/1.X";
		default: return NULL;
	}
}

static size_t parse_http_response(NU_Response_t *header, char *header_str){
	MU_LOG_TRACE(logger, "Header: \n%s", header_str);
	size_t header_size = 0;
	char *first_line;
	char *rest_of_lines;
	char *line = strtok_r(header_str, "\r\n", &rest_of_lines);
	if(!line){
		MU_LOG_WARNING(logger, "No header field found!");
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
			parse_http_version(&header->version, line);
		} else if (isdigit((unsigned char)*line)){
			parse_http_status(header, line);
		} else {
			MU_LOG_WARNING(logger, "Invalid header format!'%s'", line);
		}
	} while((line = strtok_r(NULL, " ", &first_line)));
	while((line = strtok_r(NULL, "\r\n", &rest_of_lines))){
		// strlen(line) + strlen("\r\n");
		header_size += strlen(line) + 2;
		parse_http_field(header->header, line);
	}
	// header_size + strlen("\r\n")
	return header_size += 2;
}

static size_t parse_http_request(NU_Request_t *req, char *header_str){
	MU_LOG_TRACE(logger, "Header: \n%s", header_str);
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
	// header_size + strlen("\r\n")
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
	NU_Request_t *req = calloc(1, sizeof(NU_Request_t));
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
char *NU_Response_append_header(NU_Response_t *res, const char *header, size_t *header_size){
	char header_cpy[*header_size + 1];
	snprintf(header_cpy, *header_size + 1, "%s", header);
	size_t header_read = parse_http_response(res, header_cpy);
	MU_LOG_TRACE(logger, "Read %zu of %zu bytes of header; New header size = %zu!", header_read, *header_size, *header_size - header_read);
	*header_size -= header_read;
	return (char *)header + header_read;
}

char *NU_Request_append_header(NU_Request_t *req, const char *header, size_t *header_size){
	char header_cpy[*header_size + 1];
	snprintf(header_cpy, *header_size + 1, "%s", header);
	size_t header_read = parse_http_request(req, header_cpy);
	MU_LOG_TRACE(logger, "Read %zu of %zu bytes of header; New header size = %zu!", header_read, *header_size, *header_size - header_read);
	*header_size -= header_read;
	return (char *)header + header_read;
}

/*
    Clears the response header of all fields and attributes.
*/
bool NU_Response_clear(NU_Response_t *res){
	MU_ARG_CHECK(logger, false, res);
	bool header_cleared = DS_Hash_Map_clear(res->header, free);
	if(!header_cleared){
		MU_LOG_ERROR(logger, "DS_Hash_Map_clear: 'Was unable to clear hash map!'");
		return false;
	}
	res->version = NU_HTTP_NO_VER;
	res->status = 0;
	return true;

}

bool NU_Request_clear(NU_Request_t *req){
	MU_ARG_CHECK(logger, false, req);
	bool header_cleared = DS_Hash_Map_clear(req->header, free);
	if(!header_cleared){
		MU_LOG_ERROR(logger, "DS_Hash_Map_clear: 'Was unable to clear hash map!'");
		return false;
	}
	memset(&req->path, '\0', NU_HTTP_FILE_PATH_LEN + 1);
	req->version = NU_HTTP_NO_VER;
	req->method = NU_HTTP_NO_METHOD;
	free(req->path);
	req->path = NULL;
	return true;
}

/*
    Returns the null-terminated string of the header.
*/
char *NU_Response_to_string(NU_Response_t *res){
	MU_ARG_CHECK(logger, NULL, res);
	char *buf = calloc(1, NU_HTTP_HEADER_LEN + 1);
	if(!buf){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	size_t size, i = 0, size_left = NU_HTTP_HEADER_LEN + 1;
	const char *status = (res->status > 509) ? NULL : NU_HTTP_Status_Codes[res->status];
	if(!status){
		MU_LOG_INFO(logger, "Invalid HTTP Status!");
		goto error;
	}
	char *version = http_version_to_string(res->version);
	if(!version){
		MU_LOG_INFO(logger, "Invalid HTTP Version!");
		goto error;
	}
	size_t retval = snprintf(buf, size_left, "%s %s\r\n", version, status);
	size_left -= retval;
	char **arr = DS_Hash_Map_key_value_to_string(res->header, NULL, ": ", NULL, &size, NULL);
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

	error:
		if(buf){
			free(buf);
		}
		return NULL;
}

char *NU_Request_to_string(NU_Request_t *req){
	MU_ARG_CHECK(logger, NULL, req);
	char *buf = calloc(1, NU_HTTP_HEADER_LEN + 1);
	if(!buf){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	size_t size, i = 0, size_left = NU_HTTP_HEADER_LEN + 1;
	char *method = http_method_to_string(req->method);
	if(!method){
		MU_LOG_INFO(logger, "Invalid HTTP Method!");
		goto error;
	}
	if(!*(req->path)){
		MU_LOG_INFO(logger, "Invalid File Path!");
		goto error;
	}
	char *version = http_version_to_string(req->version);
	if(!version){
		MU_LOG_INFO(logger, "Invalid HTTP Version!");
		goto error;
	}
	size_t retval = snprintf(buf, size_left, "%s %s %s\r\n", method, req->path, version);
	size_left -= retval;
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

	error:
		if(buf){
			free(buf);
		}
		return NULL;
}

bool NU_Response_set_field(NU_Response_t *res, char *field, char *values){
	MU_ARG_CHECK(logger, false, res, field, values);
	DS_Hash_Map_remove(res->header, field, free);
	DS_Hash_Map_add(res->header, field, values);
	return true;
}

bool NU_Request_set_field(NU_Request_t *req, char *field, char *values){
	MU_ARG_CHECK(logger, false, req, field, values);
	DS_Hash_Map_remove(req->header, field, free);
	DS_Hash_Map_add(req->header, field, values);
	return true;
}

bool NU_Response_remove_field(NU_Response_t *res, const char *field){
	MU_ARG_CHECK(logger, false, res, field);
	DS_Hash_Map_remove(res->header, field, free);
	return true;
}

bool NU_Request_remove_field(NU_Request_t *req, const char *field){
	MU_ARG_CHECK(logger, false, req, field);
	DS_Hash_Map_remove(req->header, field, free);
	return true;
}

char *NU_Response_get_field(NU_Response_t *res, const char *field){
	MU_ARG_CHECK(logger, false, res, field);
	char *value = DS_Hash_Map_get(res->header, field);
	return value;
}

char *NU_Request_get_field(NU_Request_t *req, const char *field){
	MU_ARG_CHECK(logger, false, req, field);
	char *value = DS_Hash_Map_get(req->header, field);
	return value;
}
