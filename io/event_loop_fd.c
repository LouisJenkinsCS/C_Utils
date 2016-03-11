#include "event_loop_fd.h"
#include "../data_structures/list.h"
#include "logger.h"
#ifdef C_UTILS_REF_COUNT
#include "../memory/ref_count.h"
#endif
#include "../misc/argument_check.h"

#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#define C_UTILS_EVENT_LOOP_NAME_MAX_LEN 64

struct c_utils_event_local_fd {
	/// Array of pipe file descriptor.
	int pipe_fds[2];
};

struct c_utils_event_source_fd {
	/// File descriptor associated with this event
	int fd;
	/// Name associated with this event.
	char name[C_UTILS_EVENT_LOOP_NAME_MAX_LEN + 1];
	/// Data from user to pass to the dispatcher
	void *user_data;
	/// The event source type (determines what events we poll for).
	enum c_utils_event_source_type type;
	/// Callback to handle dispatching the event.
	c_utils_dispatch dispatcher;
	/// Callback to handle finalizing user_data
	c_utils_finalize finalizer;
};

struct c_utils_event_loop_fd {
	/// File descriptor used to wake up from poll (when sources are added)
	int wake_fd;
	/// Atomic flag to check if we are running
	_Atomic bool running;
	/// Synchronized list of new sources to add to original list.
	struct c_utils_list *add_sources;
	/// Synchronized list of sources to remove from the original list.
	struct c_utils_list *remove_sources;
	/// Synchronized list of sources currently polling on.
	struct c_utils_list *sources;
};

static struct c_utils_logger *logger;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./io/logs/event_loop_fd.log", "w", C_UTILS_LOG_LEVEL_ALL);

static bool socket_to_non_blocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1) {
		C_UTILS_LOG_ERROR(logger, "fctnl { F_GETFL } : \"%s\"", strerror(errno));
		return false;
	}

	flags |= O_NONBLOCK;

	int err = fcntl(fd, F_SETFL, flags);
	if(err) {
		C_UTILS_LOG_ERROR(logger, "fctnl { F_SETFL } : \"%s\"", strerror(errno));
		return false;
	}

	return true;
}

/*
	Potential thread-safety issue: If we decrement the count after finalizing user_data, then
	it's possible we may end up doing it twice. When we create the destructor for it, we
	100% need to check if we have already finalized the user_data.
*/
static void finished_with_source(void *src) {
	struct c_utils_event_source_fd *source = src;
	if(source->finalizer)
			source->finalizer(source->user_data);

	#ifdef C_UTILS_REF_COUNT
	c_utils_ref_dec(src);
	#endif
}


static inline bool has_input(struct pollfd *fd) {
	return fd->revents & POLLIN;
}

static inline bool has_output(struct pollfd *fd) {
	return fd->revents & POLLOUT;
}

static inline bool has_disconnected(struct pollfd *fd) {
	return fd->revents & POLLHUP;
}

static inline bool has_error(struct pollfd *fd) {
	return fd->revents & POLLERR;
}

static inline bool is_invalid(struct pollfd *fd) {
	return fd->revents & POLLNVAL;
}

/*
	Handles dispatching the source passed source. If there is an error or if dispatcher returns
	true, then the event is consumed as well and is set to be removed from the list.
*/
static bool handled_dispatch(struct c_utils_event_source_fd *source, int flags) {
	char buf[BUFSIZ];

	int bytes_read = 0;
	if(flags & C_UTILS_EVENT_SOURCE_TYPE_READ) {
		int bytes_read = read(source->fd, buf, BUFSIZ);
		if(bytes_read <= 0) {
			if(bytes_read == -1)
				C_UTILS_LOG_ERROR(logger, "Error occuring while reading from event name: \"%s\"'s file descriptor: \"%s\"", source->name,  strerror(errno));
			else
				C_UTILS_LOG_VERBOSE(logger, "Event name: \"%s\" returned EOF!", source->name);

			return true;
		}
	}

	if(source->dispatcher(source->user_data, source->fd, buf, bytes_read, flags))
		return true;

	return false;
}

/*
	Checks to see if the passed file descriptor is bad and should be removed from the pollfd array
	ASAP. This also manages the logging of such errors as well.
*/
static inline bool is_bad_fd(struct pollfd *fd, const char *event_name) {
	if(has_error(fd))
		C_UTILS_LOG_ERROR(logger,  "Event Source: \"%s\" has had an error!", event_name);
	else if(is_invalid(fd))
		C_UTILS_LOG_WARNING(logger, "Event Name: \"%s\" does not contain a valid file descriptor!", event_name);
	else if(has_disconnected(fd))
		C_UTILS_LOG_VERBOSE(logger, "Event Name: \"%s\" has disconnected!", event_name);
	else
		return false;

	return true;
}

/*
	Where we poll the pollfd array, check them when woken to see any are ready, and if they are,
	dispatch them through their dispatcher callback.
*/
static void poll_fds(struct c_utils_event_loop_fd *loop, struct pollfd *fds, int size) {
	int retval = poll(fds, size, -1);
	if (retval == -1) {
		// Handle errors
	}

	// Since we poll indefinitely without timeout, it would be REALLY strange if it returned 0, but just in case.
	assert(retval);

	// The first file descriptor is the wake_fd.
	if (has_input(fds)) {
		// Decrement size, and if it is 0, there are no others ready.
		if(!--retval) {
			C_UTILS_LOG_TRACE(logger, "Woken up prematurely, returning from poll early...");
			return;
		}
	}

	size_t num_sources;
	struct c_utils_event_source_fd **sources = (void *) c_utils_list_as_array(loop->sources, &num_sources);

	// Skip the first as it is the wake_fd, loop while we have more left and not at end.
	for(int i = 1; retval && i <= num_sources; i++) {
		struct c_utils_event_source_fd *source = sources[i-1];

		if(is_bad_fd(fds + i, source->name)) {
			c_utils_list_add(loop->remove_sources, source, NULL);
			retval--;
			continue;
		}

		int flags = (has_input(fds + i) ? C_UTILS_EVENT_SOURCE_TYPE_READ : 0) |
				  (has_output(fds + i) ? C_UTILS_EVENT_SOURCE_TYPE_WRITE : 0);
		if (flags && handled_dispatch(source, flags)) {
			c_utils_list_add(loop->remove_sources, source, NULL);
			retval--;
		}
	}

	free(sources);
}

/*
	Since there may be data in the pipe after being woken up, we clear it out now so we do not
	continuously be woken up again, as poll is level triggered.
*/
static void flush_wake_fd(struct c_utils_event_loop_fd *loop) {
	char buf[1];

	while(read(loop->wake_fd, buf, 1) > 0);
}

/*
	Updates the current list of sources bsaed on the add_sources and remove_sources list.
*/
static void update_loop_sources(struct c_utils_event_loop_fd *loop) {
	struct c_utils_event_source_fd *source;

	// First, we add any new_sources to the list of sources.
	C_UTILS_LIST_FOR_EACH(source, loop->add_sources)
		c_utils_list_add(loop->sources, source, NULL);
	c_utils_list_clear(loop->add_sources, NULL);

	// Then we clear sources of any removed_sources.
	C_UTILS_LIST_FOR_EACH(source, loop->remove_sources)
		c_utils_list_remove(loop->add_sources, source, NULL);
	c_utils_list_clear(loop->remove_sources, finished_with_source);
}

static void create_pollfd_from_sources(struct c_utils_event_loop_fd *loop, struct pollfd **fds, int *size) {

}

/*
	The main event loop, which handles adding and removing sources, creating pollfd's from sources,
	flushing the wake_fd, and finally polling on all pollfd's until any is ready.
*/
static void start_loop(struct c_utils_event_loop_fd *loop) {
	if(!loop)
		return;

	while(loop->running) {
		// In the case that we were woken up, we clear the data on the FD.
		flush_wake_fd(loop);

		update_loop_sources(loop);

		/*
			We add the wake_fd first to guarantee ease of retrieval when we are polling. The wake_fd is used
			to wake up the event_loop to add more sources if there is no new data to poll on currently.
		*/
		int index = 0;
		struct pollfd fds[c_utils_list_size(loop->sources) + 1];
		fds[index++] = (struct pollfd) {
			.fd = loop->wake_fd,
			.events = POLLIN
		};

		/*
			And below, we add the rest of the event_source file descriptors to be polled on.
		*/
		struct c_utils_event_source_fd *source;
		C_UTILS_LIST_FOR_EACH(source, loop->sources)
			fds[index++] = (struct pollfd) {
				.fd =source->fd,
				.events = 
					((source->type & C_UTILS_EVENT_SOURCE_TYPE_READ) ? POLLIN : 0) |
				  ((source->type & C_UTILS_EVENT_SOURCE_TYPE_WRITE) ? POLLOUT : 0)

			};

		poll_fds(loop, fds, index);
	}

	c_utils_list_clear(loop->sources, finished_with_source);
}

