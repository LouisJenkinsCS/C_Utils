#include "MU_Logger.h"

static const unsigned int fnc_hash = 2087918081;
static const unsigned int fle_hash = 2087918017;
static const unsigned int lno_hash = 2087924627;
static const unsigned int msg_hash = 2087925873;

static unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

static const char *parse_token(const char *token, const char *msg, const char *file_name, const char *line_number, const char *function_name, va_list list){
	switch(hash(token)){
		case 2087918081: return function_name;
		case 2087918017: return file_name;
		case 2087924627: return line_number;
		case 2087925873: {
			char *str = malloc(1024);
			vsprintf(str, msg, list);
			return str;
		}
	}	
	return NULL;
}

static char *format_string(const char *format, const char *msg, const char *file_name, const char *line_number, const char *function_name, ...){
	char *buffer = malloc(2048);
	int buf_index = 0;
	const int token_size = 4;
	/// Since max size of each format is 3, I can just read 2 to 3 characters to determine whether or not it passes a check.
	char token[token_size + 1];
	va_list list;
	va_start(list, function_name);
	char ch;
	while((ch = *format++))
	{
		if(ch == '%'){
			token[0] = '%';
			strncpy(token + 1, format, token_size - 1);
			token[token_size] = '\0';
			const char *tok_str = parse_token(token, msg, file_name, line_number, function_name, list);
			if(tok_str){
				strncpy(buffer + buf_index, tok_str, strlen(tok_str));
				format += token_size - 1;
				buf_index += strlen(tok_str);
				continue;
			}
		}
		buffer[buf_index++] = ch;
	}
	buffer[buf_index] = '\0';
	return buffer;
}

char *MU_get_timestamp(void){
	const int buffer_size = 80;
	time_t t = time(NULL);
	struct tm *current_time = localtime(&t);
	char *time_and_date = malloc(buffer_size);
	strftime(time_and_date, buffer_size, "%I:%M:%S %p", current_time);
	return time_and_date;
}

/// Initialize logger.
MU_Logger_t *MU_Logger_create(const char *filename, const char *mode, MU_Logger_Level_e level){
	MU_Logger_t *logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("MU_Logger_create->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	logger->file = fopen(filename, mode);
	if(!logger->file) {
		MU_DEBUG("MU_Logger_create->fopen: \"%s\"\n", strerror(errno));
		goto error;
	}
	logger->level = level;
	return logger;

	error:
		if(logger){
			free(logger);
		}
		return NULL;
}

int MU_Logger_destroy(MU_Logger_t *logger){
	if(!logger) return 0;
	if(logger->file) fclose(logger->file);
	return 1;
}
