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

/*
	Checks for the following:
	A) If the timeout has ellapses and the event already triggered, hence if it should reset the timeout.
	B) If any data is needing to be prepared, if the prepare callback is specified.
	C) If any events are finished and should be removed from the list.
*/
static void event_loop_main(void *args){
	MU_Event_Source_t *source = args;
	if(MU_FLAG_GET(source->flags, event_finished)) return;
	if(source->prepare && !MU_FLAG_GET(source->flags, event_prepared)){
		source->data = source->prepare();
		MU_FLAG_SET(source->flags, event_prepared);
	}
	bool do_event = true;
	if(source->timeout){
		struct timeval curr_time;
		gettimeofday(&curr_time, NULL);
		if(timercmp(&source->next_timeout, &curr_time, <)){
			if(!timerisset(&source->next_timeout)) do_event = false;
			size_t seconds = source->timeout / 1000;
			size_t milliseconds = (source->timeout % 1000) * 1000;
			struct timeval timeout_time = { seconds, milliseconds };
			timeradd(&curr_time, &timeout_time, &source->next_timeout);
		} else do_event = false;
	}
	if(do_event){
		if(source->check == NULL || source->check(source->data)){
			if(source->dispatch(source->data)){
				if(source->finalize) source->finalize(source->data);
				MU_FLAG_SET(source->flags, event_finished);
			}
		}
	}
}

MU_Event_Source_t *MU_Event_Source_create(MU_Event_Prepare prepare_cb, MU_Event_Check check_cb, MU_Event_Dispatch dispatch_cb, MU_Event_Finalize finalize_cb, unsigned long long int timeout){
	MU_ARG_CHECK(logger, NULL, dispatch_cb);
	MU_Event_Source_t *source = calloc(1, sizeof(MU_Event_Source_t));
	if(!source){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	source->prepare = prepare_cb;
	source->check = check_cb;
	source->dispatch = dispatch_cb;
	source->finalize = finalize_cb;
	if(timeout){
		timerclear(&source->next_timeout);
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
		Linked_List_for_each(loop->sources, event_loop_main);
		usleep(10000);
	}
	return true;
}

bool MU_Event_Loop_stop(MU_Event_Loop_t *loop){
	MU_ARG_CHECK(logger, false, loop);
	atomic_store(&loop->keep_alive, false);
	return true;
}
