#ifndef C_UTILS_EVENT_LOOP_H
#define C_UTILS_EVENT_LOOP_H

#include <stdbool.h>
#include <stddef.h>

/*
	event_local encapsulates a local event file descriptor. When
	the event_local writes to it's file descriptor, the event_loop
	will react to what is written. Note that, it is possible for the
	event_loop to write back over the file descriptor. Receiving is
	up to the caller.
*/
struct c_utils_event_local_fd;

/*
	The event_source is used to manage and maintain an emission of
	events through a file descriptor. This file descriptor can be
	a networking socket (BSD), a FILE, or a local socket.

	There is an option to pass in data which will be automatically passed to it's
	dispatcher handler. This user_data can easily be used to add context and
	functionality to each event emission. For example, one could pass in a
	connection_t object as user_data, it's file descriptor, and handle any
	and all reads and writes to/from that connection in an event-based manner.
	If there is user_data passed, the finalizer will be invoked if it has been
	provided as well.

	The event source also features a reference counter, wherein which both the
	event_loop and the caller have a reference to it. At first, only the caller
	will, but during the call to add it to a event_loop, it will be incremented.
	Why? This allows the user to dispose of responsibility for managing the lifetime of
	the event_source to the event_loop when it is finished with it.
*/
struct c_utils_event_source_fd;

/*
	The event_loop polls on all file descriptors added to it's epoll file descriptor.
	Due to the inherent thread-safety, all file descriptors are added directly to it.
	The event_loop will run in it's own thread, allowing for easy asynchronous operations.

	When the file descriptor is ready, it will read from it's list of sources until it finds the
	appropriate one. When it does, it will call the source's dispatcher handler, passing it's
	user_data, the file descriptor, the data read and the length of the data. It will also
	handle finalizing the user_data by calling finalizer if it is passed, and if user_data is
	not NULL.

	Due to the underlying list using a RWLock, and it's iterator with node-correction, it will
	allow a more effiicent way of iteration through the list. Removing a the source can be done immediately
	as the node-correction algorithm will simply jump to the next one. Once can also add sources safely between
	iterations due to the iterator. Sometimes the source will be checked twice, but that is no real issue.

	When the source's dispatcher returns true, it will decrement it's own reference count. This mean, if the user
	wishes to use it in another (or even the same) event_loop at a later time, they may do so as it will not be
	destroyed until after both have relinquished their references to the source.
*/
struct c_utils_event_loop_fd;

/*
	Dispatches the event, forwarding the passed user data and the file descriptor the event
	source is composed of. Both the data read (read_data) and the length of the data (data_len)
	can be used to reconstruct the data read.

	If the type of struct represented by read_data is known, data_len may as well be ignored.
	However, in the case of a string, it can be used to idenitify a cstring.

	If the return value is true, this fd and event_source as a whole will be removed from the
	file descriptor set.
*/
typedef bool (*c_utils_dispatch)(void *user_data, int fd, void *read_data, int data_len, int rw_flags);

/*
	If user_data is passed to the event_source, then this callback will be invoked on the user_data
	when the associated event_source is removed from the event_loop.
*/
typedef void (*c_utils_finalize)(void *user_data);

enum c_utils_event_source_type {
	C_UTILS_EVENT_SOURCE_TYPE_READ = 1 << 0,
	C_UTILS_EVENT_SOURCE_TYPE_WRITE = 1 << 1
};

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_event_loop_fd event_loop_fd_t;
typedef struct c_utils_event_source_fd event_source_fd_t;
typedef struct c_utils_event_local_fd event_local_fd_t;

/*
	Enumerators
*/
#define EVENT_SOURCE_TYPE_READ C_UTILS_EVENT_SOURCE_TYPE_READ
#define EVENT_SOURCE_TYPE_WRITE C_UTILS_EVENT_SOURCE_TYPE_WRITE
#endif



#endif /* C_UTILS_EVENT_LOOP_H */