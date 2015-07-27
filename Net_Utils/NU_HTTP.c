/* TODO: Implement! */

#include <NU_HTTP.h>
#include <ctype.h>

MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("NU_HTTP.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
	logger = NULL;
}

static const int field_size = 32;
static const int value_size = 256;
static const int header_size = 4096;

static void parse_http_method(NU_Header_t *header, const char *line){
	MU_DEBUG("%s", line);
	if(strncmp(line, "GET", 3) == 0){
		header->method = NU_HTTP_GET;
	} else if(strncmp(line, "POST", 4) == 0){
		header->method = NU_HTTP_POST;
	} else if(strncmp(line, "HEAD", 4) == 0){
		header->method = NU_HTTP_HEAD;
	} else if(strncmp(line, "DELETE", 6) == 0){
		header->method = NU_HTTP_DELETE;
	} else if(strncmp(line, "TRACE", 5) == 0){
		header->method = NU_HTTP_TRACE;
	} else if(strncmp(line, "CONNECT", 7) == 0){
		header->method = NU_HTTP_CONNECT;
	} else {
		MU_LOG_WARNING(logger, "Bad HTTP method!");
	}
}

static void parse_http_path(NU_Header_t *header, const char *line){
	MU_DEBUG("%s", line);
	strncpy(header->file_path, line, 256);
}

static void parse_http_status(NU_Header_t *header, const char *line){
	MU_DEBUG("%s", line);
	int status = strtol(line, NULL, 10);
	if(!status){
		MU_LOG_WARNING(logger, "Bad HTTP status!");
		return;
	}
	header->status = status;
}

static void parse_http_version(NU_Header_t *header, const char *line){
	MU_DEBUG("%s", line);
	const int protocol_len = 8;
	if(strncmp(line, "HTTP/1.1", protocol_len) == 0){
		header->version = NU_HTTP_VER_1_1;
	} else if(strncmp(line, "HTTP/1.0", protocol_len) == 0){
		header->version = NU_HTTP_VER_1_0;
	} else if(strncmp(line, "HTTP/1.x", protocol_len) == 0){
		header->version = NU_HTTP_VER_1_X;
	} else {
		MU_LOG_WARNING(logger, "Bad HTTP version!");
	}
}

static void parse_http_header(NU_Header_t *header, const char *header_str){
	MU_DEBUG("Header: \n%s", header_str);
	const char *delimiter = ": ";
	const int delim_size = strlen(delimiter);
	char *first_line;
	char *rest_of_lines;
	char field[field_size], value[value_size];
	char *header_copy = strdup(header_str);
	char *line = strtok_r(header_copy, "\r\n", &rest_of_lines);
	if(!line){
		MU_LOG_VERBOSE(logger, "No header field found!");
		return;
	}
	/// For the first line, tokenate out the method, path (if applicable) and http version.
	line = strtok_r(line, " ", &first_line);
	do {
		if(!line){
			MU_LOG_VERBOSE(logger, "Invalid first line of header!");
			return;
		}
		if(strncmp(line, "HTTP", 4) == 0){
			parse_http_version(header, line);
		} else if (isdigit((int)*line)){
			parse_http_status(header, line);
		} else if(*line == '/'){
			parse_http_path(header, line);
		} else parse_http_method(header, line);
	} while((line = strtok_r(NULL, " ", &first_line)));
	while((line = strtok_r(NULL, "\r\n", &rest_of_lines))){
		char *offset_str = strstr(line, delimiter);
		if(!offset_str){
			MU_LOG_WARNING(logger, "No delimiter offset for line: '%s'!", line);
			return;
		}
		// A bit of math required here. We must find the length of the field to read into field buffer.
		int field_len = strlen(line) - strlen(offset_str);
		offset_str += delim_size;
		MU_DEBUG("Line: '%s'\nOffset_Str: '%s'", line, offset_str);
		snprintf(field, field_size, "%.*s", field_len, line);
		snprintf(value, value_size, "%s", offset_str);
		bool was_added = DS_Hash_Map_add(header->mapped_fields, strdup(field), strdup(value));
		if(!was_added){
			MU_LOG_WARNING(logger, "DS_Hash_Map_add: 'Was unable to add key-value pair ('%s': '%s')!'");
		}
	}
	free(header_copy);
}

NU_Header_t *NU_Header_create(void){
	NU_Header_t *header = calloc(1, sizeof(NU_Header_t));
	if(!header){
		MU_LOG_ASSERT(logger, "malloc: '%s'");
		goto error;
	}
	header->mapped_fields = DS_Hash_Map_create(31, true);
	if(!header->mapped_fields){
		MU_LOG_ERROR(logger, "DS_Hash_Map_create: 'Was unable to create hash map!'");
		goto error;
	}
	return header;

	error:
		if(header){
			free(header);
		}
		return NULL;
}

NU_Header_t *NU_Header_from(const char *header_str){
	MU_ARG_CHECK(logger, NULL, header_str);
	NU_Header_t *header = NU_Header_create();
	if(!header){
		MU_LOG_ERROR(logger, "NU_Header_create: 'Was unable to create base header!'");
		return NULL;
	}
	parse_http_header(header, header_str);
	return header;
}

char *NU_Header_to_string(NU_Header_t *header){
	char *buf = calloc(1, header_size);
	size_t size, i = 0;
	char **arr = DS_Hash_Map_key_value_to_string(header->mapped_fields, NULL, ": ", NULL, &size, NULL);
	for(; i < size; i++){
		sprintf(buf, "%s%s\r\n", buf, arr[i]);
		free(arr[i]);
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
