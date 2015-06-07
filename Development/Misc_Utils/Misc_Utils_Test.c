#include "Misc_Utils.h"
#include <stdlib.h>

int main(void){
	MU_Logger_t *logger = malloc(sizeof(MU_Logger_t));
	if(MU_Logger_Init(logger, "Misc_Utils_Test_Log.txt", "w", MU_INFO) == 0) exit(EXIT_FAILURE);
	MU_DEBUG("Testing debug macro!\n");
	MU_LOG_INFO(logger, "Testing logging info to file.\n");
	MU_LOG_ERROR(logger, "Testing logging error to file.\n");
	MU_LOG_WARNING(logger, "Testing logging error to file.\n");
	int i = 0;
	for(;i<100;i++){
		MU_LOG_VERBOSE(logger, "Increment Count: %d\n", i);
	}
	MU_LOG_INFO(logger, "Testing timer!\n");
	Timer_t *timer = Timer_Init(1);
	sleep(5);
	Timer_Stop(timer);
	char *_time = Timer_To_String(timer);
	MU_LOG_INFO(logger, "Time is %s\n", _time);
	free(_time);
	MU_LOG_INFO(logger, "Testing Assertion!\n");
	char *tmp = NULL;
	MU_ASSERT(tmp != NULL, logger);
	Timer_Destroy(timer);
	MU_Logger_Deref(logger);
	return EXIT_SUCCESS;
}