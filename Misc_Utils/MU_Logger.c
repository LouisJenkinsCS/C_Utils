#include "MU_Logger.h"

char *MU_Get_Timestamp(void){
	const int buffer_size = 80;
	time_t t = time(NULL);
	struct tm *current_time = localtime(&t);
	char *time_and_date = malloc(buffer_size);
	strftime(time_and_date, buffer_size, "%I:%M:%S %p", current_time);
	return time_and_date;
}

int MU_Logger_Init(MU_Logger_t *logger, const char *filename, const char *mode, MU_Logger_Level_t level){
	if(!logger) return 0;
	FILE *file = fopen(filename, mode);
	if(!file) {
		MU_DEBUG("Unable to create logfile!\n");
		return 0;
	}
	logger->file = file;
	logger->level = level;
	return 1;
}

int MU_Logger_Destroy(MU_Logger_t *logger, unsigned int free_ptr){
	if(!logger) return 0;
	if(logger->file) fclose(logger->file);
	if(free_ptr) free(logger);
	return 1;
}
