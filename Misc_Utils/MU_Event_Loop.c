#include <MU_Event_Loop.h>
#include <MU_Flags.h>
#include <MU_Arg_Check.h>
#include <unistd.h>

MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Misc_Utils/Logs/MU_Event_Loop.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}


static const int event_finished = 1 << 0;
static const int event_prepared = 1 << 1;
static const int event_timeout_reset = 1 << 2;

/*
	Accept listener dispatches event to the Receive listener, with NU_Connection initialized.
	Receive listener poll until information can be acquired, then it will parse HTTP header and send its response.
	Receiver listener finalizes the data when it loses connection, EPIPE or SIGPIPE.
*/
static void event_loop_proxy(void *args){
	MU_Event_Source_t *source = args;
	if(MU_FLAG_GET(source->flags, event_finished)) return;
	if(source->timeout){
		struct timeval curr_time;
		gettimeofday(&curr_time, NULL);
		if(timercmp(&source->last_check, &curr_time, >=)){
			return;
		}
		MU_FLAG_SET(source->flags, event_timeout_reset);
		//MU_DEBUG("Time: %s", ctime((const time_t *)&source->last_check.tv_sec));
	}
	if(source->dispatch(source->data)){
		if(source->finalize) source->finalize(source->data);
		MU_FLAG_SET(source->flags, event_finished);
	}
}

MU_Event_Source_t *MU_Event_Source_create(MU_Event_Prepare prepare_cb, MU_Event_Dispatch dispatch_cb, MU_Event_Finalize finalize_cb, unsigned long long int timeout){
	MU_ARG_CHECK(logger, NULL, dispatch_cb);
	MU_Event_Source_t *source = calloc(1, sizeof(MU_Event_Source_t));
	if(!source){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	source->prepare = prepare_cb;
	source->dispatch = dispatch_cb;
	source->finalize = finalize_cb;
	MU_FLAG_SET(source->flags, event_timeout_reset);
	if(timeout){
		timerclear(&source->last_check);
		source->timeout = timeout;
	}
	return source;

	error:
		if(source){
			free(source);
		}
		return NULL;
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
		MU_Event_Source_t *curr_source = NULL;
		for(curr_source = Linked_List_head(loop->sources); curr_source; curr_source = Linked_List_next(loop->sources)){
			if(!MU_FLAG_GET(curr_source->flags, event_prepared) && curr_source->prepare){
				curr_source->data = curr_source->prepare();
				MU_FLAG_SET(curr_source->flags, event_prepared);
			}
			if(curr_source->timeout && MU_FLAG_GET(curr_source->flags, event_timeout_reset)){
				struct timeval curr_time;
				gettimeofday(&curr_time, NULL);
				if(timercmp(&curr_source->last_check, &curr_time, <)){
					size_t seconds = curr_source->timeout / 1000;
					size_t milliseconds = (curr_source->timeout % 1000) * 1000;
					struct timeval timeout_time = { seconds, milliseconds };
					//MU_DEBUG("Old Time: %s\n", ctime((const time_t *)&curr_source->last_check.tv_sec));
					timeradd(&curr_time, &timeout_time, &curr_source->last_check);
					//MU_DEBUG("Adding: %s\nNew Time: %s", ctime((const time_t *)&curr_time.tv_sec),
						//ctime((const time_t *)&timeout_time.tv_sec));
					MU_FLAG_CLEAR(curr_source->flags, event_timeout_reset);
				}
			}
		}
		MU_LOG_TRACE(logger, "Initialized event source data!");
		Linked_List_for_each(loop->sources, event_loop_proxy);
		MU_LOG_TRACE(logger, "Looping through sources to dispatch!");
		usleep(50);
	}
	return true;
}

bool MU_Event_Loop_stop(MU_Event_Loop_t *loop){
	MU_ARG_CHECK(logger, false, loop);
	atomic_store(&loop->keep_alive, false);
	return true;
}
