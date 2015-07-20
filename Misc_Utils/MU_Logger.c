#include "MU_Logger.h"

char *MU_get_timestamp(void){
	const int buffer_size = 80;
	time_t t = time(NULL);
	struct tm *current_time = localtime(&t);
	char *time_and_date = malloc(buffer_size);
	strftime(time_and_date, buffer_size, "%I:%M:%S %p", current_time);
	return time_and_date;
}

/// Initialize logger.
MU_Logger_t *MU_Logger_create(const char *filename, const char *mode, MU_Logger_Level_t level){
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
