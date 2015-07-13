/* TODO: Implement! */

#include <NU_HTTP.h>

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for NU_HTTP's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "NU_HTTP_Log.txt", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}
