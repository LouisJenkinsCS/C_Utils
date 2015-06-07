#include "Misc_Utils.h"

char *Misc_Utils_Get_Timestamp(void){
	const int buffer_size = 80;
	time_t t = time(NULL);
	struct tm *current_time = localtime(&t);
	char *time_and_date = malloc(buffer_size);
	strftime(time_and_date, buffer_size, "%I:%M:%S %p", current_time);
	return time_and_date;
}

int MU_Logger_Init(MU_Logger_t logger, char *filename, char *mode, MU_Logger_Level_t level){
	if(logger.is_initialized == 1) return 0;
	FILE *file = fopen(filename, mode);
	if(!fp) return 0;
	logger.file = file;
	logger.level = level;
	logger.reference_count = 1;
	logger.decrementing_count = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(logger->decrementing_count, NULL);
	logger.is_initialized = 1;
	return logger;
}

static void Logger_Destroy(MU_Logger_t **logger){
	fclose(*logger->file);
	pthread_mutex_destroy(*logger->decrementing_count);
	free(*logger->decrementing_count);
	free(*logger);
	*logger = NULL;
}

int MU_Logger_Is_Finished(MU_Logger_t logger){
	if(logger.is_initialized != 1 || logger.reference_count <= 0) return 0;
	pthread_mutex_lock(logger->decrementing_count);
	logger->reference_count--;
	pthread_mutex_unlock(logger->decrementing_count);
	if(!logger->reference_count) Logger_Destroy(logger);
}

Timer_t *Timer_Init(int start){
	Timer_t *timer = calloc(1, sizeof(Timer_t));
	timer->start = calloc(1, sizeof(time_t));
	timer->end = calloc(1 , sizeof(time_t));
	if(start) {
		time(timer->start);
		timer->is_running = 1;
	} else timer->is_running = 0;
	timer->is_finished = 0;
	return timer;
}

int Timer_Start(Timer_t *timer){
	if(timer->is_running) return 0;
	time(timer->start);
	timer->is_running = 1;
	timer->is_finished = 0;
	return 1;
}

int Timer_Stop(Timer_t *timer){
	if(!timer->is_running || timer->is_finished) return 0;
	time(timer->end);
	timer->is_running = 0;
	timer->is_finished = 1;
	return 1;
}

char *Timer_To_String(Timer_t *timer){
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

void Timer_Destroy(Timer_t *timer){
	free(timer->start);
	free(timer->end);
	free(timer);
}