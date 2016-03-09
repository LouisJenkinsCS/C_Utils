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

struct c_utils_event_local_fd {
	/// Local file descriptor
	int fd;
};

struct c_utils_event_source_fd {
	/// File descriptor associated with this event
	int fd;
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

static void finished_with_source(void *src) {
	#ifdef C_UTILS_REF_COUNT
	c_utils_ref_dec(src);
	#endif
}

static void start_loop(struct c_utils_event_loop_fd *loop) {
	if(!loop)
		return;

	// TODO: Clear the wake file descriptor in case it contains data.

	struct c_utils_event_source_fd *source;
	
	// First, we add any new_sources to the list of sources.
	C_UTILS_LIST_FOR_EACH(source, loop->add_sources)
		c_utils_list_add(loop->sources, source, NULL);
	c_utils_list_clear(loop->add_sources, NULL);

	// Then we clear sources of any removed_sources.
	C_UTILS_LIST_FOR_EACH(source, loop->remove_sources)
		c_utils_list_remove(loop->add_sources, source, NULL);
	c_utils_list_clear(loop->remove_sources, finished_with_source);

	int index = 0;
	struct pollfd fds[c_utils_list_size(loop->sources) + 1];

	// And we add the wake file descriptor.
	fds[index++] = (struct pollfd) {
		.fd = loop->wake_fd,
		.events = POLLIN
	};

	// Finally, we can add each loop 
	C_UTILS_LIST_FOR_EACH(source, loop->sources)
		fds[index++] = (struct pollfd) {
			.fd =source->fd,
			.events = 
				(source->type & C_UTILS_EVENT_SOURCE_TYPE_READ ? POLLIN : 0) |
			  (source->type & C_UTILS_EVENT_SOURCE_TYPE_WRITE ? POLLOUT : 0)

		};

	/*
		TODO: Pick up here. Below we will begin polling on ALL file descriptors in fds, and if
		either are picked, we must iterate through loop->sources to find the matching file descriptor.
		We definitely SHOULD (HINT HINT, LAZY SELF!!!) implement the iterator for the map to allow for
		easier streaming. That comes later, however, and for it should be adequate.

		When we wake up from poll, we can take advantage of the fact that the array has the same index
		as the sources in the list, hence there should only be a one time a O(N) sweet. Not too shabby.

		Also, when we wake up, we SHOULD (read: MUST!) read BUFSIZ bytes of data, for each source. We may
		want to add another field to event_source to hold this data. Doing it this way allows all polled-on
		sockets to have a round-robin like scheduler for its tasks. This unfortunately means that really busy
		file descriptors will prevent the loop from going forward, but at least all currently available 
		file descriptors can be served.
	*/
}

