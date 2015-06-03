#include "Misc_Utils.h"
#include <stdlib.h>

int main(void){
	FILE *fp = fopen("Misc_Utils_Test_Log.txt", "a+");
	MU_DEBUG("Testing debug macro!\n");
	MU_LOG_INFO(fp, "Testing logging info to file.\n");
	MU_LOG_ERROR(fp, "Testing logging error to file.\n");
	MU_LOG_WARNING(fp, "Testing logging error to file.\n");
	char *tmp = NULL;
	MU_LOG_INFO(fp, "Testing timer!\n");
	Timer_t *timer = Timer_Init(1);
	sleep(5);
	Timer_Stop(timer);
	char *_time = Timer_To_String(timer);
	MU_LOG_INFO(fp, "Time is %s\n", _time);
	free(_time);
	//MU_CHECK(tmp, EXIT_FAILURE, "Temp is NULL!\n");
	MU_ASSERT(tmp != NULL, fp);
	fclose(fp);
	return EXIT_SUCCESS;
}