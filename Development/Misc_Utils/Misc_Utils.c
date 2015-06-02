#include "Misc_Utils.h"
#include <stdlib.h>


char *Misc_Utils_Get_Timestamp(void){
	const int buffer_size = 80;
	time_t t = time(NULL);
	struct tm *current_time = localtime(&t);
	char *time_and_date = malloc(buffer_size);
	strftime(time_and_date, buffer_size, "%I:%M:%S %p", current_time);
	return time_and_date;
}

Timer_t *Timer_Init(int start){
	Timer_t *timer = calloc(1, sizeof(Timer_t));
	timer->start = calloc(1, sizeof(time_t));
	timer->end = calloc(1 , sizeof(time_t));
	if(start) time(timer->start);
	return timer;
}

int Timer_Start(Timer_t *timer){
	time(timer->start);
	return 1;
}

int Timer_Stop(Timer_t *timer){
	time(timer->end);
	return 1;
}

char *Timer_To_String(Timer_t *timer){
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