#include "MU_Logger.h"

struct c_utils_log_format_t{
	char *all_f;
	char *trace_f;
	char *verbose_f;
	char *info_f;
	char *custom_f;
	char *event_f;
	char *warning_f;
	char *error_f;
	char *assertion_f;
	char *default_f;
};

struct c_utils_logger_t {
	/// The log file to write to.
	FILE *file;
	/// The formatter for log strings.
	struct c_utils_log_format_t format;
	/// The level determining what messages will be printed.
	enum c_utils_log_level_e level;
};

struct c_utils_log_format_t {
	const char *msg;
	const char *file_name;
	const char *line_number;
	const char *function_name;
	const char *log_level;
	const char *cond_str;
	unsigned char should_free;
};

static const int buffer_size = 1024;

static const int num_of_tokens = 7;

static const char *get_function_name(struct c_utils_log_format_info_t *info, va_list args){
	return info->function_name;
}

static const char *get_line_number(struct c_utils_log_format_info_t *info, va_list args){
	return info->line_number;
}

static const char *get_log_level(struct c_utils_log_format_info_t *info, va_list args){
	return info->log_level;
}

static const char *get_file_name(struct c_utils_log_format_info_t *info, va_list args){
	return info->file_name;
}

static const char *get_message(struct c_utils_log_format_info_t *info, va_list args){
	char *str = malloc(1024+1);
	vsnprintf(str, 1024, info->msg, args);
	info->should_free = 1;
	return str;
}

static const char *get_condition(struct c_utils_log_format_info_t *info, va_list args){
	return info->cond_str;
}

static const char *get_timestamp(struct c_utils_log_format_info_t *info, va_list args){
	info->should_free = 1;
	const int buffer_size = 80;
	time_t t = time(NULL);
	struct tm *current_time = localtime(&t);
	char *time_and_date = malloc(buffer_size);
	strftime(time_and_date, buffer_size, "%I:%M:%S %p", current_time);
	return time_and_date;
}

/*
	Below, we map each token to it's function callback equivalent. I.E, %msg retrieves the
	message, hence the callback get_message would be invoked.
*/
struct {
	const char *token;
	const char *(*callback)(c_utils_log_format_info_t *info, va_list);
} static format_tokens[] = {
	{ "%fnc", get_function_name }, { "%fle", get_file_name }, 
	{ "%lno", get_line_number }, { "%lvl", get_log_level },
	{ "%msg", get_message }, { "%cnd", get_condition },
	{ "%tsm", get_timestamp }
};

static const char *parse_token(const char *token, struct c_utils_log_format_info_t *info, va_list args){
	int i = 0;
	for(; i < num_of_tokens; i++){
		if(strcmp(token, format_tokens[i].token) == 0){
			return format_tokens[i].callback(info, args);
		}
	}
	return NULL;
}

static const char *log_level_to_string(enum c_utils_log_level_e level){
	switch(level){
		case LOG_LEVEL_ASSERTION: return "ASSERTION";
		case LOG_LEVEL_ERROR: return "ERROR";
		case LOG_LEVEL_WARNING: return "WARNING";
		case LOG_LEVEL_EVENT: return "EVENT";
		case LOG_LEVEL_INFO: return "INFO";
		case LOG_LEVEL_VERBOSE: return "VERBOSE";
		case LOG_LEVEL_TRACE: return "TRACE";
		default: return NULL;
	}
}

static int format_string(const char *format, char *buffer, int buf_size, struct c_utils_log_format_info_t *info, va_list args){
	int buf_index = 0, left = buf_size;
	const int token_size = 4;
	/// Since max size of each format is 3, I can just read 2 to 3 characters to determine whether or not it passes a check.
	char token[token_size + 1];
	char ch;
	while((ch = *format++))
	{
		if(!left) break;
		info->should_free = 0;
		if(ch == '%'){
			sprintf(token, "%%%.*s",  token_size - 1, format);
			const char *tok_str = parse_token(token, info, args);
			if(tok_str){
				int should_copy = strlen(tok_str) > left ? left : strlen(tok_str);
				strncpy(buffer + buf_index, tok_str, left);
				format += token_size - 1;
				buf_index += should_copy;
				left -= should_copy;
				if(info->should_free) free((char *)tok_str);
				continue;
			}
		}
		buffer[buf_index++] = ch;
		left--;
	}
	buffer[buf_index] = '\0';
	return buf_index;
}

static const char *get_log_format(struct c_utils_logger_t *logger, enum c_utils_log_level_e level){
	if(!logger) return NULL;
	struct c_utils_log_format_t format = logger->format;
	char *real_format = NULL;
	switch(level){
		case LOG_LEVEL_ASSERTION: real_format = format.assertion_f; break;
		case LOG_LEVEL_ERROR: real_format = format.error_f; break;
		case LOG_LEVEL_WARNING: real_format = format.warning_f; break;
		case LOG_LEVEL_EVENT: real_format = format.event_f; break;
		case LOG_LEVEL_CUSTOM: real_format = format.custom_f; break;
		case LOG_LEVEL_INFO: real_format = format.info_f; break;
		case LOG_LEVEL_VERBOSE: real_format = format.verbose_f; break;
		case LOG_LEVEL_TRACE: real_format = format.trace_f;
		default: real_format = NULL;
	}
	if(!real_format){
		real_format = format.all_f;
		if(!real_format){
			real_format = format.default_f;
		}
	}
	return real_format;
}

/// Initialize logger.
struct c_utils_logger_t *c_utils_logger_create(const char *filename, const char *mode, enum c_utils_log_level_e level){
	c_utils_logger_t *logger = calloc(1, sizeof(c_utils_logger_t));
	if(!logger){
		MU_DEBUG("c_utils_logger_create->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	logger->file = fopen(filename, mode);
	if(!logger->file) {
		MU_DEBUG("c_utils_logger_create->fopen: \"%s\"\n", strerror(errno));
		goto error;
	}
	logger->level = level;
	logger->format.assertion_f = strdup("%tsm [%lvl](%fle:%lno) %fnc(): \nCondition: \"%cnd\"\nMessage: \"%msg\"\n");
	logger->format.event_f = strdup("%tsm [%lvl] %cnd: \"%msg\"\n");
	logger->format.default_f = "%tsm [%lvl](%fle:%lno) %fnc(): \n\"%msg\"\n";
	return logger;

	error:
		if(logger){
			free(logger);
		}
		return NULL;
}

bool c_utils_logger_log(struct c_utils_logger_t *logger, enum c_utils_log_level_e level, const char *custom_level, const char *msg, const char *cond, const char *file_name, const char *line_number, const char *function_name, ...){
	if(!logger || !logger->file || logger->level > level) return false;
	va_list args;
	va_start(args, function_name);
	struct c_utils_log_format_info_t info = { .msg = msg, .log_level = custom_level ? custom_level : log_level_to_string(level), .file_name = file_name, .line_number = line_number, .function_name = function_name, .cond_str = cond };
	const int log_buf_size = buffer_size * 2;
	char buffer[log_buf_size + 1];
	format_string(get_log_format(logger, level), buffer, log_buf_size, &info, args);
	fprintf(logger->file, "%s", buffer);
	fflush(logger->file);
	if(level == LOG_LEVEL_ASSERTION){
		MU_DEBUG("ASSERTION FAILED: \n%s", buffer);
	}
	return true;
}


bool c_utils_logger_destroy(struct c_utils_logger_t *logger){
	if(!logger) return false;
	if(logger->file) fclose(logger->file);
	free(logger->format.all_f);
	free(logger->format.trace_f);
	free(logger->format.verbose_f);
	free(logger->format.info_f);
	free(logger->format.custom_f);
	free(logger->format.event_f);
	free(logger->format.warning_f);
	free(logger->format.error_f);
	free(logger->format.assertion_f);
	free(logger);
	return true;
}
