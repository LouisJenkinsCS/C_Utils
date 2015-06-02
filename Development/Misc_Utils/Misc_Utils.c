#include "Misc_Utils.h"
#include <time.h>


char *Misc_Utils_Get_Timestamp(void){
	time_t current_time = time(NULL);
	const int seconds_in_half_day = 43200;
	int hours = 0, minutes = 0, seconds = 0;
	char[2] meridiem = current_time >= seconds_in_half_day ? "PM" : "AM";
	while((int)(current_time/3600)){
		hours++;
		total_time -= 3600;
	}
	while((int)(current_time/60)){
		minutes++;
		total_time -= 60;
	}
	while((int)current_time){
		seconds++;
		total_time--;
	}
	char *formatted_time;
	asprintf(&formatted_time, "%02d:%02d:%02d %s", hours, minutes, seconds, meridiem);
	return formatted_time;
}