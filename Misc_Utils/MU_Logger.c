#include "MU_Logger.h"

typedef struct {
	const char *msg;
	const char *file_name;
	const char *line_number;
	const char *function_name;
	const char *log_level;
	const char *cond_str;
	unsigned char should_free;
} MU_Logger_Format_Info_t;

static const int buffer_size = 1024;

static const char *MU_Logger_Format_Tokens[] = {
	"%fnc", "%fle", "%lno", "%lvl", "%msg", "%cnd", "%tsm"
};

static const int num_of_tokens = sizeof(MU_Logger_Format_Tokens) / sizeof(MU_Logger_Format_Tokens[0]);

static const char *MU_Logger_Format_function_name_cb(MU_Logger_Format_Info_t *info, va_list args){
	return info->function_name;
}

static const char *MU_Logger_Format_line_number_cb(MU_Logger_Format_Info_t *info, va_list args){
	return info->line_number;
}

static const char *MU_Logger_Format_log_level_cb(MU_Logger_Format_Info_t *info, va_list args){
	return info->log_level;
}

static const char *MU_Logger_Format_file_name_cb(MU_Logger_Format_Info_t *info, va_list args){
	return info->file_name;
}

static const char *MU_Logger_Format_message_cb(MU_Logger_Format_Info_t *info, va_list args){
	char *str = malloc(1024+1);
	vsnprintf(str, 1024, info->msg, args);
	info->should_free = 1;
	return str;
}

static const char *MU_Logger_Format_condition_cb(MU_Logger_Format_Info_t *info, va_list args){
	return info->cond_str;
}

static const char *MU_Logger_Format_timestamp_cb(MU_Logger_Format_Info_t *info, va_list args){
	info->should_free = 1;
	return MU_get_timestamp();
}

const char *(*MU_Logger_Format_cb[])(MU_Logger_Format_Info_t *, va_list) = {
	MU_Logger_Format_function_name_cb, MU_Logger_Format_file_name_cb, MU_Logger_Format_line_number_cb,
	MU_Logger_Format_log_level_cb, MU_Logger_Format_message_cb, MU_Logger_Format_condition_cb,
	MU_Logger_Format_timestamp_cb
};

static const char *parse_token(const char *token, MU_Logger_Format_Info_t *info, va_list args){
	int i = 0;
	for(; i < num_of_tokens; i++){
		if(strcmp(token, MU_Logger_Format_Tokens[i]) == 0){
			return MU_Logger_Format_cb[i](info, args);
		}
	}
	return NULL;
}

static int format_string(const char *format, char *buffer, int buf_size, MU_Logger_Format_Info_t *info, va_list args){
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
	MU_Logger_t *logger = calloc(1, sizeof(MU_Logger_t));
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
	logger->format.default_f = "%tsm [%lvl](%fle:%lno) %fnc(): \"%msg\"\n";
	return logger;

	error:
		if(logger){
			free(logger);
		}
		return NULL;
}

bool MU_Logger_Log(MU_Logger_t *logger, MU_Logger_Level_e level, const char *custom_level, const char *msg, const char *cond, const char *file_name, const char *line_number, const char *function_name, ...){
	if(!logger || !logger->file || logger->level > level) return false;
	va_list args;
	va_start(args, function_name);
	MU_Logger_Format_Info_t info = { .msg = msg, .log_level = custom_level ? custom_level : MU_Logger_Level_to_string(level), .file_name = file_name, .line_number = line_number, .function_name = function_name };
	const int log_buf_size = buffer_size * 2;
	char buffer[log_buf_size + 1];
	format_string(MU_Logger_Format_get(logger, level), buffer, log_buf_size, &info, args);
	fprintf(logger->file, "%s", buffer);
	if(level == MU_ASSERTION){
		MU_DEBUG("%s", buffer);

	}
	fflush(logger->file);
	return true;
}

const char *MU_Logger_Level_to_string(MU_Logger_Level_e level){
	switch(level){
		case MU_ASSERTION: return "ASSERTION";
		case MU_ERROR: return "ERROR";
		case MU_WARNING: return "WARNING";
		case MU_INFO: return "INFO";
		case MU_VERBOSE: return "VERBOSE";
		default: return NULL;
	}
}

const char *MU_Logger_Format_get(MU_Logger_t *logger, MU_Logger_Level_e level){
	if(!logger) return NULL;
	MU_Logger_Format_t format = logger->format;
	char *real_format = NULL;
	switch(level){
		case MU_ASSERTION: real_format = format.assertion_f;
		case MU_ERROR: real_format = format.error_f;
		case MU_WARNING: real_format = format.warning_f;
		case MU_CUSTOM: real_format = format.custom_f;
		case MU_INFO: real_format = format.info_f;
		case MU_VERBOSE: real_format = format.verbose_f;
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

int MU_Logger_destroy(MU_Logger_t *logger){
	if(!logger) return 0;
	if(logger->file) fclose(logger->file);
	free(logger->format.all_f);
	free(logger->format.info_f);
	free(logger->format.assertion_f);
	free(logger->format.error_f);
	free(logger->format.verbose_f);
	free(logger->format.custom_f);
	free(logger->format.warning_f);
	return 1;
}
