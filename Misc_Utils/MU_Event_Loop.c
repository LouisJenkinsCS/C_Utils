#include <MU_Event_Loop.h>
#include <MU_Arg_Check.h>

MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Misc_Utils/Logs/MU_Event_Loop.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}


/*
	Accept listener dispatches event to the Receive listener, with NU_Connection initialized.
	Receive listener poll until information can be acquired, then it will parse HTTP header and send its response.
	Receiver listener finalizes the data when it loses connection, EPIPE or SIGPIPE.
*/
static void event_loop_proxy(void *args){
	MU_Event_Source_t *source = args;
	if(source->dispatch(source->data)){
		if(source->finalize) source->finalize(source->data);
		source->is_finished = true;
	}
}

MU_Event_Source_t *MU_Event_Source_create(MU_Event_Prepare prepare_cb, MU_Event_Dispatch dispatch_cb, MU_Event_Finalize finalize_cb){
	MU_ARG_CHECK(logger, NULL, dispatch_cb);
	MU_Event_Source_t *source = calloc(1, sizeof(MU_Event_Source_t));
	if(!source){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	source->prepare = prepare_cb;
	source->dispatch = dispatch_cb;
	source->finalize = finalize_cb;
	source->data = malloc(sizeof(void *));
	if(!source->data){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		free(source);
		return NULL;
	}
	return source;
}

bool MU_Event_Source_destroy(MU_Event_Source_t *source){
	MU_ARG_CHECK(logger, false, source);
	free(source);
	return true;
}

MU_Event_Loop_t *MU_Event_Loop_create(void){
	MU_Event_Loop_t *loop = calloc(1, sizeof(MU_Event_Loop_t));
	if(!loop){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	loop->sources = Linked_List_create();
	if(!loop->sources){
		MU_LOG_ERROR(logger, "Linked_List_create: 'Was unable to create Linked List!");
		free(loop);
		return NULL;
	}
	loop->keep_alive = ATOMIC_VAR_INIT(false);
	return loop;
}

bool MU_Event_Loop_add(MU_Event_Loop_t *loop, MU_Event_Source_t *source){
	MU_ARG_CHECK(logger, false, loop, source);
	return Linked_List_add(loop->sources, source, NULL);
}

bool MU_Event_Loop_run(MU_Event_Loop_t *loop){
	MU_ARG_CHECK(logger, false, loop);
	atomic_store(&loop->keep_alive, true);
	while(atomic_load(&loop->keep_alive)){
		MU_Event_Source_t *curr_source = Linked_List_head(loop->sources);
		do {
			if(!curr_source->is_prepared && curr_source->prepare){
				curr_source->data = curr_source->prepare();
				curr_source->is_prepared = true;
			}
		} while((curr_source = Linked_List_next(loop->sources)));
		MU_LOG_TRACE(logger, "Initialized event source data!");
		Linked_List_for_each(loop->sources, event_loop_proxy);
		MU_LOG_TRACE(logger, "Looping through sources to dispatch!");
		pthread_yield();
	}
	return true;
}
