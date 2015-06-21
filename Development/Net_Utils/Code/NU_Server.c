__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for NU_Server's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "NU_Server_Log.txt", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

NU_Server_t *NU_Server_create(int flags){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	return server;
}
