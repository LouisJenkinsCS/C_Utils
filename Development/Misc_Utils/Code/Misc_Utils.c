#include "Misc_Utils.h"

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

MU_Timer_t *MU_Timer_Init(int start){
	MU_Timer_t *timer = calloc(1, sizeof(MU_Timer_t));
	timer->start = calloc(1, sizeof(time_t));
	timer->end = calloc(1 , sizeof(time_t));
	if(start) {
		time(timer->start);
		timer->is_running = 1;
	} else timer->is_running = 0;
	timer->is_finished = 0;
	return timer;
}

int MU_Timer_Start(MU_Timer_t *timer){
	if(timer->is_running) return 0;
	time(timer->start);
	timer->is_running = 1;
	timer->is_finished = 0;
	return 1;
}

int MU_Timer_Stop(MU_Timer_t *timer){
	if(!timer->is_running || timer->is_finished) return 0;
	time(timer->end);
	timer->is_running = 0;
	timer->is_finished = 1;
	return 1;
}

char *MU_Timer_To_String(MU_Timer_t *timer){
	if(!timer->is_finished) return NULL;
	double total_time = difftime(*timer->end, *timer->start);
	int hours = 0, minutes = 0, seconds = 0;
	while((int)(total_time/3600)){
		hours++;
		total_time -= 3600;
	}
	while((int)(total_time/60)){
		minutes++;
		total_time -= 60;
	}
	while((int)total_time){
		seconds++;
		total_time--;
	}
	char *formatted_time;
	asprintf(&formatted_time, "%02d:%02d:%02d", hours, minutes, seconds);
	return formatted_time;
}

void MU_Timer_Destroy(Timer_t *timer){
	free(timer->start);
	free(timer->end);
	free(timer);
}