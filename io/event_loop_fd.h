#ifndef C_UTILS_EVENT_LOOP_H
#define C_UTILS_EVENT_LOOP_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define C_UTILS_EVENT_LOOP_NAME_MAX_LEN 64

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

	The ready_flags passed is used to identify which events are ready, based on the polled information.
	For example, if only write is available, EVENT_SOURCE_TYPE_WRITE will be passed.

	The returned flags help modify the underlying event source for this event. For example, if one
	were to register for EVENT_SOURCE_TYPE_READ and EVENT_SOURCE_TYPE_WRITE events, one would receive
	both read and write events, even when you can only write after the next read. A way to circumvent
	this is to return EVENT_FLAGS_WRITE_DONE, which will remove the EVENT_SOURCE_TYPE_WRITE flag from
	the types of input this source polls for. Then after receiving input, we may send EVENT_FLAGS_WRITE
	to re-enable writing again, and optionally EVENT_FLAGS_READ_DONE to disable reading. Lastly, if we are
	finally finished with this event, return EVENT_FLAG_DONE, or optionally EVENT_FLAGS_READ_DONE | EVENT_FLAGS_WRITE_DONE.
	If no modifications are desired, return EVENT_FLAG_NONE, or 0.
*/
typedef enum c_utils_event_flags (*c_utils_dispatch)(void *user_data, int fd, void *read_data, int data_len, int ready_flags);

/*
	If user_data is passed to the event_source, then this callback will be invoked on the user_data
	when the associated event_source is removed from the event_loop.
*/
typedef void (*c_utils_finalize)(void *user_data);

/*
	The type of event the source will poll for, and handle. These will also be passed as
	flags to help determine which of the events are ready. For example, if you pass
	EVENT_SOURCE_TYPE_READ | EVENT_SOURCE_TYPE_WRITE, and the file descriptor is only available
	for EVENT_SOURCE_TYPE_READ, yet you need to collect data to write, what you can pass until the
	next round of polling begins.

	Remember that poll is level-triggered, so until you can no longer write, the next poll will begin
	immediately. Hence, you may want to write until all data is filled, then consume the event by returning
	true in the dispatcher, otherwise resources are wasted waking up repeatedly.

	The flags EOF is normally only set internally, however if it is passed as an initial flag, it will
	only read once if read flag is passed.
*/
enum c_utils_event_source_type {
	C_UTILS_EVENT_SOURCE_TYPE_READ = 1 << 0,
	C_UTILS_EVENT_SOURCE_TYPE_WRITE = 1 << 1,
	C_UTILS_EVENT_SOURCE_TYPE_EOF = 1 << 2
};

/*
	Event flags returned from a dispatch to modify the underlying event source.
*/
enum c_utils_event_flags {
	C_UTILS_EVENT_FLAGS_NONE = 0,
	C_UTILS_EVENT_FLAGS_DONE = 1 << 0,
	C_UTILS_EVENT_FLAGS_WRITE = 1 << 1,
	C_UTILS_EVENT_FLAGS_WRITE_DONE = 1 << 2,
	C_UTILS_EVENT_FLAGS_READ = 1 << 3,
	C_UTILS_EVENT_FLAGS_READ_DONE = 1 << 4
};

/*
	Defaults:
		name:
			"FD: [FILE_DESCRIPTOR]"
		user_data:
			NULL
		type:
			READ | WRITE
		finalizer:
			NULL
		ref_counted:
			false
		close_fd:
			false
		logger:
			NULL
*/
struct c_utils_event_source_fd_conf {
	/// Name associated with this event.
	char name[C_UTILS_EVENT_LOOP_NAME_MAX_LEN + 1];
	/// Data from user to pass to the dispatcher
	void *user_data;
	/// The event source type (determines what events we poll for).
	enum c_utils_event_source_type type;
	/// Callback to handle finalizing user_data
	c_utils_finalize finalizer;
	/// If we reference count this event source.
	bool ref_counted;
	/// If we should close file descriptor on destruction
	bool close_fd;
	/// Logger used to log any tracing debug and errors to.
	struct c_utils_logger *logger;
};


/*
	Convenience macro to create an event source from a passed file, returning the result through
	src. The event_name will be the name of file when it was opened, and the file descriptor
	being the actual file descriptor associated with file.
*/
#define C_UTILS_EVENT_SOURCE_FROM(src, file, user_data, dispatcher, finalizer, flags) \
do { \
	if(!file) \
		goto err; \
	\
	int fd = fileno(file); \
	if(fd == -1)  \
    	goto err; \
    \
    char *proc_link; \
    asprintf(&proc_link, "/proc/self/fd/%d", fd); \
    if(!proc_link) \
    	goto err; \
    \
    char filename[BUFSIZ + 1]; \
    int retval = readlink(proc_link, filename, BUFSIZ); \
    if(retval < 0) \
    	goto err_filename; \
    filename[retval] = '\0'; \
    \
  	src = c_utils_event_source_fd_create(filename, fd, user_data, dispatcher, finalizer, flags); \
  	\
  	free(proc_link); \
  	break; \
  	\
  	err_filename: \
  		free(proc_link); \
	err: \
		src = NULL; \
		break; \
} while(0)

/*
	Creates an event source that is polled on for local writes to fd. fd should NOT be a valid file descriptor,
	it is in fact created and the passed fd is set to the created one. If there is an error, src will be null and
	fd will be -1. Let me repeat, do NOT pass an actual file descriptor, because it will be replaced. 

	Note that the flags are not modifiable nor configurable. This is because pipes offer unidirectional streams of
	data, and hence we the event source can only read from the socket, while the returned_fd is always the writing end.
*/
#define C_UTILS_EVENT_SOURCE_LOCAL(src, returned_fd, event_name, user_data, dispatcher, finalizer) \
do { \
	int fds[2]; \
	\
	int retval = pipe(fds); \
	if(retval == -1) \
		goto err; \
	\
	returned_fd = fds[1]; \
	src = c_utils_event_source_fd_create(event_name, fds[0], user_data, dispatcher, finalizer, C_UTILS_EVENT_SOURCE_TYPE_READ); \
	if(!src) \
		goto err; \
	\
	break; \
	\
	err: \
		src = NULL; \
		returned_fd = -1; \
		\
		break; \
} while(0)

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_event_loop_fd event_loop_fd_t;
typedef struct c_utils_event_source_fd event_source_fd_t;
typedef struct c_utils_event_local_fd event_local_fd_t;

/*
	Macros
*/
#define EVENT_SOURCE_FROM(...) C_UTILS_EVENT_SOURCE_FROM(__VA_ARGS__)
#define EVENT_SOURCE_LOCAL(...) C_UTILS_EVENT_SOURCE_LOCAL(__VA_ARGS__)

/*
	Enumerators
*/
#define EVENT_SOURCE_TYPE_READ C_UTILS_EVENT_SOURCE_TYPE_READ
#define EVENT_SOURCE_TYPE_WRITE C_UTILS_EVENT_SOURCE_TYPE_WRITE
#define EVENT_SOURCE_TYPE_EOF C_UTILS_EVENT_SOURCE_TYPE_EOF
#define EVENT_SOURCE_TYPE_DONE C_UTILS_EVENT_SOURCE_TYPE_DONE

/*
	Functions
*/
#define event_loop_fd_create(...) c_utils_event_loop_fd_create(__VA_ARGS__)
#define event_loop_fd_add(...) c_utils_event_loop_fd_add(__VA_ARGS__)
#define event_loop_fd_remove(...) c_utils_event_loop_fd_remove(__VA_ARGS__)
#define event_source_fd_create(...) c_utils_event_source_fd_create(__VA_ARGS__)
#define event_source_fd_destroy(...) c_utils_event_source_fd_destroy(__VA_ARGS__)
#endif

struct c_utils_event_loop_fd *c_utils_event_loop_fd_create();

bool c_utils_event_loop_fd_add(struct c_utils_event_source_fd *source);

bool c_utils_event_loop_fd_remove(struct c_utils_event_source_fd *source);

struct c_utils_event_source_fd *c_utils_event_source_fd_create(char *event_name, int fd, void *user_data,
	c_utils_dispatch dispatcher, c_utils_finalize finalizer, enum c_utils_event_source_type flags);

void c_utils_event_source_fd_destroy(struct c_utils_event_source_fd *source);

void c_utils_event_loop_fd_destroy(struct c_utils_event_loop_fd *loop);

#endif /* C_UTILS_EVENT_LOOP_H */