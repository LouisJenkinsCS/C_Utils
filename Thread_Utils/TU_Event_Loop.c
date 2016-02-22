#include <TU_Event_Loop.h>
#include <TU_Flags.h>
#include <TU_Arg_Check.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;
static MU_Logger_t *event_logger = NULL;

MU_LOGGER_AUTO_CREATE(logger, "./Misc_Utils/Logs/TU_Event_Loop.log", "w", TU_ALL);

MU_LOGGER_AUTO_CREATE(event_logger, "./Misc_Utils/Logs/TU_Event_Loop_Events.log", "w", TU_ALL);


static const int event_finished = 1 << 0;
static const int event_prepared = 1 << 1;

/*
	Checks for the following:
	A) If the timeout has ellapses and the event already triggered, hence if it should reset the timeout.
	B) If any data is needing to be prepared, if the prepare callback is specified.
	C) If any events are finished and should be removed from the list.
*/
static void event_loop_main(void *args){
	TU_Event_Source_t *source = args;
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

TU_Event_Source_t *TU_Event_Source_create(TU_Event_Prepare prepare_cb, TU_Event_Check check_cb, TU_Event_Dispatch dispatch_cb, TU_Event_Finalize finalize_cb, unsigned long long int timeout){
	MU_ARG_CHECK(logger, NULL, dispatch_cb);
	TU_Event_Source_t *source = calloc(1, sizeof(TU_Event_Source_t));
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

bool TU_Event_Source_destroy(TU_Event_Source_t *source){
	MU_ARG_CHECK(logger, false, source);
	free(source);
	return true;
}

TU_Event_Loop_t *TU_Event_Loop_create(void){
	TU_Event_Loop_t *loop = calloc(1, sizeof(TU_Event_Loop_t));
	if(!loop){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	loop->sources = DS_List_create(true);
	if(!loop->sources){
		MU_LOG_ERROR(logger, "Linked_List_create: 'Was unable to create Linked List!");
		goto error;
	}
	loop->finished = TU_Event_create("Finished", event_logger, 0);
	if(!loop->finished){
		MU_LOG_ERROR(logger, "TU_Event_create: 'Was unable to create Finished event!");
		goto error;
	}
	loop->keep_alive = ATOMIC_VAR_INIT(false);
	return loop;

	error:
		if(loop){
			free(loop->sources);
			free(loop->finished);
		}
		return NULL;
}

bool TU_Event_Loop_add(TU_Event_Loop_t *loop, TU_Event_Source_t *source){
	MU_ARG_CHECK(logger, false, loop, source);
	return DS_List_add(loop->sources, source, NULL);
}

bool TU_Event_Loop_run(TU_Event_Loop_t *loop){
	MU_ARG_CHECK(logger, false, loop);
	atomic_store(&loop->keep_alive, true);
	while(atomic_load(&loop->keep_alive)){
		DS_List_for_each(loop->sources, event_loop_main);
		usleep(10000);
	}
	TU_Event_signal(loop->finished, 0);
	return true;
}

bool TU_Event_Loop_stop(TU_Event_Loop_t *loop){
	MU_ARG_CHECK(logger, false, loop);
	atomic_store(&loop->keep_alive, false);
	return true;
}

bool TU_Event_Loop_destroy(TU_Event_Loop_t *loop, bool free_sources){
	MU_ARG_CHECK(logger, false, loop);
	TU_Event_Loop_stop(loop);
	TU_Event_wait(loop->finished, -1, 0);
	DS_List_destroy(loop->sources, free_sources ? free : NULL);
	free(loop);
	return true;
}
