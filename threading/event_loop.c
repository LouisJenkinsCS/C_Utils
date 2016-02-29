#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>

#include "../data_structures/list.h"
#include "../threading/events.h"
#include "../threading/event_loop.h"
#include "../misc/flags.h"
#include "../misc/argument_check.h"
#include "../io/logger.h"

struct c_utils_event_loop {
	/// Maintains list of sources to check.
	struct c_utils_list_t *sources;
	/// Keep-Alive flag
	_Atomic bool keep_alive;
	/// The event to wait on for it to finish.
	struct c_utils_event_t *finished;
};

/*
	void*::c_utils_event_prepare(Void); Prepares the event.
	bool::c_utils_event_check(Void*); Checks if event is ready. 
	bool::c_utils_event_dispatch(Void*); Dispatches the event.
	bool::c_utils_event_finalize(Void*); Destroys the data if it is finished.
*/
struct c_utils_event_source {
	/// Data returned from an event.
	void *data;
	/// Callback used to prepare the data passed to it.
	c_utils_event_prepare prepare;
	/// Callback used to check if event is ready.
	c_utils_event_check check;
	/// Callback used to dispatch said event.
	c_utils_event_dispatch dispatch;
	/// Callback used whenever the main loop finishes.
	c_utils_event_finalize finalize;
	/// Internal flags used to help maintain information about the event source.
	unsigned int flags;
	/// The timeval used to record the absolute time of the next timeout.
	struct timeval next_timeout;
	/// The timeout the timeval will be set to after triggering.
	unsigned int timeout;
};

static struct c_utils_logger_t *logger = NULL;
static struct c_utils_logger_t *event_logger = NULL;

LOGGER_AUTO_CREATE(logger, "./threading/logs/event_loop.log", "w", C_UTILS_LOG_LEVEL_ALL);
LOGGER_AUTO_CREATE(event_logger, "./threading/logs/event_loop_events.log", "w", C_UTILS_LOG_LEVEL_ALL);


static const int event_finished = 1 << 0;
static const int event_prepared = 1 << 1;

/*
	Checks for the following:
	A) If the timeout has ellapses and the event already triggered, hence if it should reset the timeout.
	B) If any data is needing to be prepared, if the prepare callback is specified.
	C) If any events are finished and should be removed from the list.
*/
static void event_loop_main(void *args) {
	struct c_utils_event_source *source = args;

	if (FLAG_GET(source->flags, event_finished)) return;
	
	if (source->prepare && !FLAG_GET(source->flags, event_prepared)) {
		source->data = source->prepare();
		FLAG_SET(source->flags, event_prepared);
	}
	
	bool do_event = true;
	if (source->timeout) {
		struct timeval curr_time;
		gettimeofday(&curr_time, NULL);
		
		if (timercmp(&source->next_timeout, &curr_time, <)) {
			if (!timerisset(&source->next_timeout)) do_event = false;
			size_t seconds = source->timeout / 1000;
			size_t milliseconds = (source->timeout % 1000) * 1000;
			struct timeval timeout_time = { seconds, milliseconds };
			timeradd(&curr_time, &timeout_time, &source->next_timeout);
		} else do_event = false;
	}
	if (do_event) {
		if (source->check == NULL || source->check(source->data)) {
			if (source->dispatch(source->data)) {
				if (source->finalize) source->finalize(source->data);
				FLAG_SET(source->flags, event_finished);
			}
		}
	}
}

struct c_utils_event_source *c_utils_event_source_create(c_utils_event_prepare prepare_cb, c_utils_event_check check_cb, c_utils_event_dispatch dispatch_cb, c_utils_event_finalize finalize_cb, unsigned long long int timeout) {
	C_UTILS_ARG_CHECK(logger, NULL, dispatch_cb);

	struct c_utils_event_source *source = calloc(1, sizeof(struct c_utils_event_source));
	if (!source) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	
	source->prepare = prepare_cb;
	source->check = check_cb;
	source->dispatch = dispatch_cb;
	source->finalize = finalize_cb;
	if (timeout) {
		timerclear(&source->next_timeout);
		source->timeout = timeout;
	}
	
	return source;

	error:
		if (source)  
			free(source);
		
		return NULL;
}

bool c_utils_event_source_destroy(struct c_utils_event_source *source) {
	C_UTILS_ARG_CHECK(logger, false, source);
	
	free(source);
	return true;
}

struct c_utils_event_loop *c_utils_event_loop_create(void) {
	struct c_utils_event_loop *loop = calloc(1, sizeof(struct c_utils_event_loop));
	if (!loop) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	
	// Synchronized list.
	loop->sources = c_utils_list_create(true);
	if (!loop->sources) {
		C_UTILS_LOG_ERROR(logger, "c_utils_list_create: 'Was unable to create list of sources!");
		goto error;
	}
	
	loop->finished = c_utils_event_create("Finished", event_logger, 0);
	if (!loop->finished) {
		C_UTILS_LOG_ERROR(logger, "c_utils_event_create: 'Was unable to create \"Finished\" event!");
		goto error;
	}
	
	loop->keep_alive = ATOMIC_VAR_INIT(false);
	return loop;

	error:
		if (loop) {
			free(loop->sources);
			free(loop->finished);
		}
		return NULL;
}

bool c_utils_event_loop_add(struct c_utils_event_loop *loop, struct c_utils_event_source *source) {
	C_UTILS_ARG_CHECK(logger, false, loop, source);
	
	return c_utils_list_add(loop->sources, source, NULL);
}

bool c_utils_event_loop_run(struct c_utils_event_loop *loop) {
	C_UTILS_ARG_CHECK(logger, false, loop);
	
	atomic_store(&loop->keep_alive, true);
	while (atomic_load(&loop->keep_alive)) {
		c_utils_list_for_each(loop->sources, event_loop_main);
		usleep(10000);
	}
	
	c_utils_event_signal(loop->finished, 0);
	return true;
}

bool c_utils_event_loop_stop(struct c_utils_event_loop *loop) {
	C_UTILS_ARG_CHECK(logger, false, loop);
	
	atomic_store(&loop->keep_alive, false);
	return true;
}

bool c_utils_event_loop_destroy(struct c_utils_event_loop *loop, bool free_sources) {
	C_UTILS_ARG_CHECK(logger, false, loop);
	
	c_utils_event_loop_stop(loop);
	c_utils_event_wait(loop->finished, -1, 0);
	
	c_utils_list_destroy(loop->sources, free_sources ? free : NULL);
	free(loop);
	return true;
}
