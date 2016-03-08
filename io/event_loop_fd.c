#include "event_loop_fd.h"
#include "../data_structures/list.c"

#include <stdint.h>
#include <stdatomic.h>

struct c_utils_event_local_fd {
	/// Local file descriptor
	int fd;
};

struct c_utils_event_source_fd {
	/// File descriptor associated with this event
	int fd;
	/// Data from user to pass to the dispatcher 
	void *user_data;
	/// Callback to handle dispatching the event. 
	c_utils_dispatch dispatcher;
	/// Callback to handle finalizing user_data
	c_utils_finalize finalizer;
	/// Atomic reference counter, so user can destroy their own source.
	_Atomic int ref_count;
};

struct c_utils_event_loop_fd {
	/// Epoll's file descriptor
	int epfd;
	/// Synchronized list of sources to poll on.
	struct c_utils_list *sources;
};