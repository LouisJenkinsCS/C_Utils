# Utilities Package for the C Programming Language

##Summary

Utilities Package for the C Programming Language, dubbed C_Utils, contains a plethora of varying, but very useful abstractions for the modern day C programmer. It is designed to be easy to use and easy to read, and very newbie-friendly. As the C standard library is extremely minimal, I decided to create my own abstractions and package them to share them with others who may desire to use them. 

This project originally began as a way to teach myself the C Programming language, and so I am mainly self-taught. This may lead to some "weird" semantics and style choices, however this has been remedied as of March of 2016, by redesigning the entire library, to increase readability and overall control flow.

As of now, this project will be continued to be worked on and expanded on as I desire to learn new things. The amount of time spent developing this package has bordered on 800 hours now, and I do not think I'll ever be truly finished, as there are nearly infinitely many amount of things I can do. 

It should be noted once again, while this project is my "personal playground" does not mean it will be unusable to others.

##Development Stages

[<b>Unimplemented</b>] - I have not begun to implement this yet, but I do plan on doing so at a later date.

[<b>In Development</b>] - I am currently working on the implementation of this, and I should be finished soon.

[<b>Unstable</b>] - I have finished developing this, and have done minor testing, but have not done enough to determine if it is bug-free yet.

[<b>Stable</b>] - I have finished developing this, and have done repeated tests to ensure it's stability, however I may add some more features later on at my leisure.

[<b>Finished</b>] - I have finished developing this, and have done repeated tests to ensure it's stability, and I do not plan on adding anything more than bug-fixes in the future.

##Stable vs Development Branches

Just to get this out of the way early in the README, as of yet, C_Utils has gone through a MAJOR refactoring, and even now as I write this it is not finished, and hence the stable version does NOT accurately reflect the code samples, or even the libraries within this package. Hence, I urge that, if not already, see https://github.com/theif519/C_Utils/tree/development. 

##Artificial Namespace patterns

To avoid the issue of namespace collision, as C has only one namespace, all libraries in this package contain the C_UTILS_ prefix for macros, and c_utils_ prefix for functions and structs. Now, this can look rather ugly. For example...

~~~c

struct c_utils_logger *logger;
C_UTILS_LOGGER_AUTO_CREATE(logger, ...);

struct c_utils_thread_pool *pool = c_utils_thread_pool_create(...);

~~~

There's no dodging around it, the c_utils_ prefix makes everything more long-winded. However, this is necessary when writing libraries like these. While it may be unwieldy, maybe you think you don't have to worry about collisions for  a logger or a thread_pool because no other library you use has them. This is where the NO_C_UTILS_NO_PREFIX define comes in. If you define this before importing the libraries, it will strip the c_utils prefix through macro defines, typedef most (99%) of the library the name, ended with a "_t". For example...

~~~c

#define NO_C_UTILS_PREFIX
#include <logger.h>
#include <thread_pool.h>

logger_t *logger;
LOGGER_AUTO_CREATE(logger, ...);

thread_pool_t *pool = thread_pool_create(...);

~~~

Now it is a LOT less long-winded, and much more elegant looking. This trade off adds the issue of potential collision, so be warned. Because of this conciseness, all below code samples use the NO_C_UTILS_PREFIX, and so contain no c_utils_ prefix. 

##Lifetime Management

All objects returned from this library can optionally be reference counted. This will work using the lock-free allocator wrapper provided in memory/ref_count.h, to allocate the structure using it. This drastically eases the usage and management the libraries in this package.

##Configurations

Recently, I found that just adding extra parameters won't fit the bill any longer, and so I decided to begin to rewrite everything again, implementing a new way to creating an object, whether that be a list, a thead_pool, or an event.

This new way is simulator to the Configuration Object pattern, or well, it pretty much is. The configuration object will allow the user to fine-tune the objects during construction, such as assigning it a specific logger to log to (so no more logging to about 30 different files), if it should be synchronized (so no extra boolean parameter in every constructor), and other context-specific options. Each will also have its own default behavior, allowing for shorter and easier construction.

###Code Sample

~~~c

/*
    The below elegantly demonstrates the importance of the configuration 
    object. As can be seen, the object allows an almost JSON-like appearence to
    configuring the specifics of the structure. This allows an fine-tuned 
    data structure to fit the needs of the user. Unlike preprocessor macros,
    these are evaluated at run-time, meaning you can have different configured
    objects of the same type (which with preprocessor #define blocks would
    only allow one). This comes with the trade-off of more memory for each,
    as the underlying data structure must keep track of each field 
    independently. 
*/

map_conf_t conf =
{
    .flags = MAP_CONCURRENT | MAP_RC_INSTANCE | MAP_SHRINK_ON_TRIGGER | MAP_DELETE_ON_DESTROY,
    .num_buckets = 128,
    .callbacks = 
    {
        .destructors = 
        {
            .key = free,
            .value = destroy_value
        },
        .hash_function = my_custom_hash,
        .value_comparator = my_custom_comparator
    },
    .growth = 
    {
        .ratio = 1.5,
        .trigger = .75
    },
    .shrink =
    {
        .ratio = .75,
        .trigger = .1
    },
    .obj_len = sizeof(struct my_obj),
    .logger = my_logger
};

~~~

##Libraries Packages

###Threading

Provides utilities, abstractions and tools which help automate/manage multithreading,

#### Thread Pool [<b>Stable</b>] Version: 1.3

thread_pool_t is a thread pool with it's own priority queue for tasks. As implied by the use of a priority queue, tasks may be submitted via 6 different priorities, Lowest, Low, Medium, High and Highest. High Priority tasks would jump ahead of tasks of Low priority, intuitively. 

The static thread pool maintains a steady amount of threads, never growing or shrinking in size, however unused threads will block, hence it will not waste resources waiting for a new task to be submitted. 

Each task can return an asynchronous result, which, based on my implementation of events, you may wait (or poll) for when the task finishes. So, to reiterate, a task, by default, returns a result which can be waited on.

When submitting tasks, it comes with it's own default priority and will return a result_t result to wait on, but by passing certain flags, like HIGH_PRIORITY | NO_RESULT you may flag tasks specifically.

Finally you can pause the thread pool, meaning, that currently running tasks finish up, but it will not run any more until after either a timeout elapses or the call to resume is made.

Another note to mention is that the thread pool showcases the use of MU_Events, as waiting on a result is an event, so is to pause and resume. 

#####Code Sample

```c

/// Initialize a pool of 10 threads, enough to handle anything without wasting resources.
static const size_t pool_size = 10;

/// Example task for thread pool to run.
static void *task_example(void *args);

thread_pool_t *tp = thread_pool_create(pool_size);

/// Adds a task of medium priority (default), with no argument (NULL) nor flags (0)
result_t *result = thread_pool_add(tp, task_example, NULL, 0);
/// Adds the same task of high priority, no argument, and without a result.
thread_pool_add(tp, task_example, NULL, THIGH_PRIORITY | NO_RESULT);
/// Note that you do not need to worry about cleaning up the result from TP_NO_RESULT.

/// Wait on result with no timeout (-1).
void *retval = result_get(result, -1);
/// Destroy the result, as it no longer is needed.
result_destroy(result);

/// Pause the thread pool for 5 seconds.
thread_pool_pause(tp, 5);
/// Wait on Thread Pool to finish everything.
thread_pool_wait(tp, -1);

/// Destroy thread pool.
thread_pool_destroy(tp);

```

####Scoped Locks [<b>In Development</b>] Version: .5

An implementation of a C++-like scope_lock. The premise is that locks should be managed on it's own, and is finally made possible using GCC and Clang's compiler attributes, __cleanup__. The locks supported so far are pthread_mutex_t, pthread_spinlock_t, and sem_t. It will lock when entering the scope, and unlock when leaving (or in the case of sem_t, it will increment the count, and then decrement). This abstracts the need for the need to lock/unlock the lock, as well as generifying the type of lock used as well, as the allocation is done using C11 generics. Hence, the type of underlying lock is type-agnostic.

#####Code Sample

```c

static logger_t *logger;

/*
    Allocation
*/
// If we want a spinlock?
scoped_lock_t *lock_1 = scoped_lock_spinlock(0, logger);
// What if we want to create one from an already existing lock?
pthread_mutex_t *lock;
scoped_lock_t *lock_2 = SCOPED_LOCK_FROM(lock);
// What if we do not want a lock at times?
scoped_lock_t *lock_3 = scoped_lock_no_op();

/*
    Scoped Locking
*/
// Regardless of type, it will work the same.
scoped_lock_t *s_lock;

// Single-line
SCOPED_LOCK(s_lock)
    do_something();

// Multi-line
SCOPED_LOCK(s_lock) {
    do_something();
    do_something_else();
    if (is_something) {
        // Note, we return without needing to unlock.
        return;
    }
    finally_do_something();
}

/*
    Sometimes the compilers throws a warning (or error) because you return inside of the scoped_lock block. This can be mitigated with the C_UTILS_UNACCESSEIBLE macro.
*/

C_UTILS_UNACCESSIBLE;

```

That's it, and now the need to lock is abstracted completely. Note as well that we can replace pthread_mutex_t with a pthread_spinlock_t and it would work the exact same, because everything else is managed in the background.

####Conditional Locks [<b>Stable</b>] Version: 1.0

Features auto-logging locking macros for mutexes and rwlocks. It simply checks if the lock if NULL before attempting to lock, as attempting to lock a NULL pthread_*_t argument will cause a segmentation fault. Also should note that if something goes wrong, I.E on EDEADLK, it will log the precise location of said errors.

Now imagine you have a data structure that uses rwlocks, or even mutexes. Now, the overhead of a mutex, no matter how optimized they are, is still unneeded on single threaded applications for said data structure. Hence, if lock is NULL it will result in a NOP, and do nothing. The compiler may even optimize away the check entirely and act like it's not there, who knows. The point being that it allows for more flexible data structures which can't be made lockless.

#####Code Sample

```c

/// Assume this gets initialized before being called.
pthread_rwlock_t *lock;
/// Etc.
COND_RWLOCK_RDLOCK(lock, logger);
/// Later, maybe in some other thread...
COND_RWLOCK_WRLOCK(lock, logger);

```

####Events [<b>Stable</b>] Version: 1.2

An implementation of Win32 Events. As of yet, it allows you to wait on an event, which is equivalent to waiting on a condition variable, signaled by other threads. 

events allows you to wait on events, and supports flags which allow you to set the default state, whether or not to signal the event after a timeout, and whether or not to auto-reset the event after a thread exits the event, or after the last waiting thread leaves. 

events is an abstraction on top of a pthread_mutex, pthread_cond variable, and other flags. MU_Events are entirely thread safe and efficient, and also entirely flexible, coming with it's own MU_Logger support. You can also name events and pass the thread identifier to allow debugging said events easier.

#####Code Sample

```c

/// Logger for events. Assume it gets initialized and setup before calling events.
static logger_t *event_logger;
/*
    The event object used for signaling and waiting on events.
    This event is named "Test Event" and logs to the event logger,
    inituitively. It is signaled by default, hence those calling to wait on it
    will return immediately. The first thread to leave this event
    successfully, will reset the event to non-signaled state.
*/
event_t *event = event_create("Test Event", event_logger, TU_EVENT_SIGNALED_BY_DEFAULT | TU_EVENT_AUTO_RESET);
/// We now to want wait on this event. Thread identifier can be anything, but lets just use pthread_self.
event_wait(event, -1, (unsigned int) pthread_self());
/// Now some other thread signals this...
event_signal(event, (unsigned int) pthread_self());
/* 
    Now, we're done with said event. Destroy it. Note that if any threads are 
    waiting on it, they are woken up and can gracefully exit.
*/
event_destroy(event, (unsigned int) pthread_self());

```

####Event Loop [<b>Unstable</b>] Version 0.6

event_loop, is a simple, minimal event loop, which allows you to add event sources, and have them be polled on at regular intervals. The Event Loop also has support for timed or timer events, upon which the dispatch callback will be called when it's timeout ellapses.

The Event Loop is rather simple and bare bones for now, polling once every 10ms, hence the amount of precision is very high, yet somewhat expensive, however not too much so, but does not scale well when idle and does better when it has a lot of tasks to do/poll for.

The Event Loop takes a prepare callback (to prepare any such data to be passed to an event when ready), a check callback (to check if the event is ready), a dispatch callback (to notify any threads waiting on the event), and finally a finalize callback (to destroy the user data when it is finished).

#####Code Sample

```c

void *prepare_event(void *data) {
    return malloc(sizeof(struct some_event_t));
}

bool check_event(void *data) {
    return do_something_with(data)
}

bool dispatch_event(void *data) {
    notify_thread_waiting_on(data);
    return true;
}

bool finalize_event(void *data) {
    free(data);
    return true;
}

bool print_something(void *data) {
    printf("Something!\n");
    return true;
}

/*
    The below is an example of how to use the event loop.
    It creates a simple event with it's appropriate callbacks,
    then sets it's timeout to 0, meaning it is polled on once every
    10ms. Next is a timed_event, which is will print "Something!" once
    every 10 seconds.
*/
int main(void) {
    event_source_t *event = event_source_create(prepare_event, check_event, dispatch_event, finalize_event, 0);
    event_source_t *timed_event = event_source_create(NULL, NULL, print_something, NULL, 10);

    event_loop_t *loop = event_loop_create();
    
    event_loop_add(loop, event);
    event_loop_add(loop, timed_event);
    event_loop_run(loop);
    
    return 0;
}

```

It's very simple, and as always, easy to use. Right now it's not entirely stable, but I'm working on fixing bugs and adding appropriate features.

###Memory Management

Memory Management Utilities provide useful tools and abstractions which will help automate, or at the very least, improve quality-of-life for a C programmers, which can be helpful to an amateur or a professional. All of the below are thread safe.

####Hazard Pointers [<b>Unstable</b>]

Provides a flexible and easy to use implementation of hazard pointers, described in the research paper by Maged M. Michael, [here](http://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf).

The implementation is still in development and needing of testing, however I have an optimistic outlook on how it will look. Basically, you will "acquire" data via it's pointer and "release" it when you are finished. You must make sure to release the reference once finished with it, through the API calls hazard_acquire() and hazard_release and hazard_release_all.

Another notable feature is that you do not need to keep your own reference to the hazard pointer itself, as it's allocated as thread-local storage and allocated on first use. Lastly, any remaining data not freed before the program ends, will be destroyed when the library is unlinked (I.E program termination).

#####Code Sample

```c

/*
    For an example, lets assume the structure of a basic
    lock-free stack.
*/
Stack_t *stack;
/*
    Lets emulate a simple pop lockless procedure.
*/
Node_t *head, *next;
while (true) {
    head = stack->head;
    if (!head) 
        return NULL;
    
    hazard_acquire(0, head);
    if (head != stack->head) {
        pthread_yield();
        continue;
    }

    next = head->next;
    if (__sync_bool_compare_and_swap(&stack->head, head, next)) 
        break;

    pthread_yield();
}
void *data = head->item;
// true = retire data, set to be deleted later, false = just remove reference
hazard_release(head, true);

```

####Reference Counting Memory Allocator [<b>Unimplemented</b>]

#####Planned

* A simple memory allocator that wraps malloc and maintains a reference count
    - Reference count should go down whenever the wrapped free function is called.
* Each allocated item has it's own destructor
    - Registered by user
        + Defaults to free.

####Object Pool [<b>Unimplemented</b>]

#####Planned

* Recycles a recyclable object to be used later.
    - A callback is used to inform and manipulate the objects themselves to restore them to a state that can be reused.
* Manages objects
    - Creates new ones when it is empty and requires more
        + Through callbacks
    - Destroys old ones when they have not been used for a while
        + Through callbacks.

###String Utilities

Supplies basic string manipulations that are intuitive and easy to use.

####String Manipulations [<b>Stable</b>] Version: 2.1

A basic, yet very powerful and conventional string manipulations library. Supports ASCII strings only, and some functions support the use of non NULL-terminated functions.

From simple string reversal or splitting and joining a string based on a delimiter, or even dynamic concatenation of strings, is all included in this library. This library fixes and improves upon the standard libc and glibc library by adding functionality that is sorely missing, in an efficient manner.

There is also a convenience typedef for cstrings, String, which abstracts the need to use pointers. Lastly, there is a convenience macro that can be used to handle memory management of non-constant strings, TEMP, which utilitizes the GCC or Clang's compiler attributes.

#####Code Sample

```c

/*
    The below demonstrates the ease of use of declaring a string with the
    typedef provided. Alternatively, you can declare it as char *str, which
    can be used interchangeably.
*/
string str = "Hello World";

/*
    The below demonstrates the memory management of strings being handled by
    the compiler, automatically being destroyed when it leaves the scope of the
    block of code.
*/
string TEMP str = strdup("Hello World");

/*
    Now, on to the actual functions of this string manipulation library.
    First, we will attempt to reverse a portion of the string, str, declared
    above. We only wish to reverse "World" however, so we will make use of
    pointer arithmetic to get the offset of the string. We want to reverse
    everything after after "Hello ", so we pass 0 as the length to specify
    that it is null terminated and that strlen can be used.
*/
string_reverse(str + 6, 0);

/*
    Now, for the next example, imagine we have a fixed amount, but a somewhat
    large amount of strings to concatenate together, and you not only wish to
    concatenate them together, but also apply some kind of delimiter. For
    instance, SU_String_split can split an array into an array of strings
    based on a delimiter, and you wish to join them together with a new
    delimiter. While SU_String_replace can do the job just as well (better),
    lets assume you actually modify the array of strings somehow. You have
    two options here, either SU_String_join, which is easier, but you need to
    pass an array, but what if you wish to append a new string, then you have
    resize the array (if it's not constant), or create an entirely new one.
    Instead, SU_STRING_CONCAT_ALL allows you to concatenate any number of
    strings with an optional delimiter.
*/
string storage;
STRING_CONCAT_ALL(&storage, ",", str, "How are you today", "Good I hope", "Good day!");

/*
    The other functions are rather straight forward, however to go more into 
    STRING_CONCAT_ALL, notice you do not need to add a NULL dummy parameter
    or specify the size. That's because the preprocessor can determine it for
    you, which it does.
*/
```

####String Buffer [<b>In Development</b>]

Abstracts away the need to manually allocate strings and do tedious string manipulations. The string_buffer automatically manages resizing itself and shrinking when needed. It features a generic macro (requires C11 _Generic keyword) to automatically append, prepend or insert any of the standard types. It is also optionally thread-safe.

The string_buffer supports an option to enable synchronizaiton, which is done through a spinlock. Majority of cases do not require synchronization, however if ever you have a case where you require one, for say a producer-consumer relationship, it can be enabled easily. 

#####Code Sample

~~~c

// Allocate with initial value with no synchronization
string_buffer_t *str_buf = string_buffer_create("Hello World", false);

// Append strings
string_buffer_append(str_buf, ", I am ");

// Append numbers
STRING_BUFFER_APPEND(str_buf, 22);

// But wait, there's a better way to do this...
string_buffer_clear(str_buf);

// Lets append all from one macro!
STRING_BUFFER_APPEND_FORMAT(str_buf, "Hello World, I am %d years old!", 22);

// Now lets delete Hello World
string_buffer_delete(str_buf, 0, 12);

// And remove the "old!" part
string_buffer_delete(str_buf, STRING_BUFFER_END - 3, STRING_BUFFER_END);

// And retrieve so we can display it.
char *str = string_buffer_get(str_buf);
puts(str);

~~~

####Regular Expressions [<b>Unimplemented</b>]

#####Planned

* Easy abstracions for regular expressions
    - No need for cleaning up anything
* Special regex printf function
    - Use printf with regex to determine what to select from a passed string

###I/O

Brings useful abstractions when dealing with streams through file descriptors. Buffering (I.E line-by-line), to asynchronous reading/writing without needing to worry if it is a FILE or socket file descriptor. Also features a configurable logging utility.

####Logger [<b>Stable</b>] Version: 1.5

A minimal logging utility which supports logging based on log levels, with it's own custom formatting. Also supports a custom log level with custom log label for formatting. Supports the usage of the __constructro__ and __destructor__ compiler attributes (available with Clang and GCC) to automatically manage the lifetime of the logger, through the LOGGER_AUTO_CREATE macro.

Below is an example of a custom format. This is the default logging format used when no custom format has been provided.

"%tsm \[%lvl\](%fle:%lno) %fnc(): \n\"%msg\"\n"

Would produce the following:

9:39:32 PM \[INFO\](test_file:63) main():
"Hello World!"

The currently implemented log format tokens are...

%tsm: Timestamp (HH/MM/SS AM/PM)
%lvl: Log Level
%fle: File
%lno: Line Number
%fnc: Function
%msg: Message
%cond: Condition (Used for assertions)

#####Code Sample

```c

/*
    Instantiation
*/
static logger_t *logger; 
LOGGER_AUTO_CREATE(logger, "Test_File.txt", "w", LOG_LEVEL_ALL);

/*
    Usage
*/
LOG_INFO(logger, "Hello %s", "World");
DEBUG("Hello World");
ASSERT(1 == 0, logger, "1 != 0!");

```

#####Planned Features

* Configuration File support
* Logging to a set of loggers rather than just one
    - Allows a group of loggers with different log levels to be logged to
    - Allows late registration and unregistration for injection
* More Log Format Tokens

####Event Polling [<b>Unimplemented</b>]

#####Planned

* Create event_sources from file descriptors
    - Use local sockets to emit local events
    - Monitor non-local sockets
    - Asynchronously read files
* event_sources allows for registering callbacks to handle events
    - Passes the userpassed data and the file descriptor
    - Allows for asynchronous handling of events.
* event_poller(?) created in another thread which manages itself
    - Handles event_sources submitted
* Goal
    - Allow for truely easy and effortless asynchronicity.

####File Buffering [<b>Unimplemented</b>]

#####Planned

* Stream over a file by buffering a certain amount of it at a time
    - Will support buffering by line.
        + Effortless next_line and prev_line abstractions

####Asyncronous Streams [<b>Unimplemented</b>]

#####Planned

* Stream over an abitrary collection of items
    - strings 
    - lists
    - maps
    - arrays
    - etc.
* Streams can be updated concurrently as they are being taken from
    - Easy Producer-Consumer

###Networking Utilities

Provides basic networking utilities which allow a developer with almost no experience with sockets to manage connections, send/receive data, etc. Also features two managers and recycling pools for connections, allowing for efficient use.

####Connection [<b>Stable</b>] Version: 1.1

connection is the base file for all transactions on sockets and between endpoints. It contains all sending and receiving functions and logs everything. It can be created manually through connection_create() and connection_init() but it's best created through the abstractions, server and client. 

#####Code Samples

```c

/// Unlike normal bsd socket functions, my abstractions provide a timeout.
const int timeout = 60;
/// The flags to be passed to send().
const int send_flags = 0;
/// The flags to be passed to recv().
const int recv_flags = 0;

connection_t *conn;
/// Assume it's been setup and configured and already.
char buf[BUFSIZ];

size_t bytes_sent = connection_send(conn, buf, BUFSIZ, timeout, send_flags);
assert(bytes_sent);

// Simple error checking, assertions are easy for small programs.
size_t bytes_received = connection_receive(conn, buf, BUFSIZ, timeout, recv_flags);
assert(bytes_sent);

printf("%.*s", (int)bytes_received, buf);

```

####Server [<b>Stable</b>] Version: 1.0

The philosophy of the server is that it acts as a manager for connection objects, which are completely configured and connected to a client. The server also manages a resource and recycling pool of connection objects as well for after you disconnect them. The server also allows you to manage multiple bindings of ports. 

#####Code Sample

```c

/// The initial connection pool size
const int connection_pool = 10;
/// The initial bound socket pool size.
const int bound_socket_pool = 4;
/// Whether or not locks are initialized. 0 for single-threaded or care multithreading.
const int is_threaded = 1;
/// The IP address to bind to. If it is NULL, it is bound INADDR_ANY
const char *ip_addr = "127.0.0.1";
/// Port to bind to.
const unsigned int port = 8000;
/// Unlike normal bsd socket functions, my abstractions provide a timeout.
const int timeout = 60;

server_t *server = server_create(connection_pool, bound_socket_pool, is_threaded);
/// connection_pool used as backlog too.
socket_t *bsock = server_bind(server, connection_pool, port, ip_addr);
connection_t *conn = server_accept(server, bsock, timeout);

```

And that's it. When you are finished with bsock, call server_unbind(server, bsock). When you are finished with the connection, call server_disconnect(server, conn), and it will add them back to the resource pool so they can be reused.

####Client [<b>Stable</b>] Version: 1.0

The philosophy of client follows server, in that it acts as a manager for connection objects, completely configured and initialized. Unlike the client, obviously, there is no need to bind to a port or create a bound socket type object. 

#####Code Sample

```c

/// The initial connection pool size
const int connection_pool = 10;
/// Whether or not locks are initialized. 0 for single-threaded or care multithreading.
const int is_threaded = 1;
/// The host's IP Address.
const char *ip_addr = "127.0.0.1";
/// Port to bind to.
const unsigned int port = 8000;
/// Unlike normal bsd socket functions, my abstractions provide a timeout.
const int timeout = 60;

client_t *client = client_create(connection_pool, is_threaded);
connection_t *conn = client_connect(client, ip_addr, port, timeout);

```

####HTTP [<b>Unstable</b>] Version 0.5

http is a simple yet minimal parsing, and generating, HTTP library. It allows you to parse HTTP requests and responses, as well generate your own by setting fields, resposne statuses, etc. by the use of it's API.

http is split between two objects: response_t and request_t, both of which take a buffer, not having to be NULL-terimainted, and returns what's left in the buffer after it parses out the rest. Hence, if you pass both the HTTP header and the message body, it will return the offset (note here) of where the message body begins. It allows you to check if a field is set by using a hash table of it's field-value pairs, file path, response status, HTTP version, etc. 

#####Code Sample

```c

/// Assume header gets filled out by some request.
char header[BUFSIZ];
size_t request_size;
request_t *req = request_create();
request_append_header(req, header, &request_size);
FILE *file = fopen(req->path, "r");
response_t *res = response_create();
/*
    Note that it takes a rather elegant looking key-value pair, in the guise
    of a struct with two char * members. What the macro does, in gist, is
    that it takes (field_t) { x, y }, into { x, y } by converting it for you.
    Hence (field_t) { "Content-Length", file_size } becomes a much better:
    { "Content-Length", file_size }.
*/
RESPONSE_WRITE(res, status, HTTP_VER_1_0,
 { "Content-Length", get_page_size(file) },
 { "Content-Type", content_type }
 );
char *response = response_to_string(res);

```

It's rather simple and elegant (in the creator's biased opinion).

###Data Structures

####Iterator [<b>In Development</b>] Version: 0.5

A general-use iterator allowing generic iteration over multiple different data structures which support it's operations. It's very callback heavy, but is filled out via the data structures which create them. 

Each implementation of the iterator have node-validation, as in even though the node may have been removed from the list between uses, it will attempt to correct itself as best it can, at worst starting over at the beginning of the data structure.

Will support generic use, as in, being able to iterate through linked lists, vectors, even hash tables without having to concern over the actual data type. I.E, containing an array of iterators for different data structures, allowing polymorphic operations.

There is also a macro to help maintain the lifecycle of the iterator using GCC and Clang's (__cleanup__) attributes. The declarations are abstracted behind AUTO_ITERATOR macro.

#####Code Sample

```c

/*
    Imagine that the below data structures are initialized already, containing strings.
*/
list_t *list;
vector_t *vec;
map_t *map;

/*
    Obtain the iterator of each in an array of iterators, like below.
*/
iterator_t *it[] = { 
    list_iterator(list), 
    vector_iterator(vec), 
    map_iterator(map) 
};

for (int i = 0; i < 3; i++) {
    char *str;
    ITERATOR_FOR_EACH(str, it)
        puts(str);
}

```

####Linked List [<b>Stable</b>] Version: 1.2

A simple, yet robust double linked list implementation. It is thread-safe, and with the use of read-write locks, allows for very efficient read-often-write-rarely uses, but it's also good for general usage as well. 

It features a way to sort the list through the use of comparators, a for-each callback and macro that can be called on all items in the list, a print-all function to print everything in a neat, formatted way, and an implementation for iterator_t. 

#####Code Sample

```c

int comparator(void *item_one, void *item_two);

const bool synchronized = true;
void *item, *item_two;

list_t *list = list_create(synchronized);
// Assume item was already allocated and points to a valid piece of memory.
list_add(list, item, NULL);
// We added the item to the list, unsorted. The third argument is a callback to add in sorted order.
list_add(list, item_two, comparator);

void *tmp;
// Loop for_each
LIST_FOR_EACH(tmp, list)
    do_something_with(tmp);

// The list is "Smart" enough to keep track of if an unsorted item was added, and will sort the list for you.
list_get(list, 0);

list_remove(list, 1);

list_destroy(list, free);

```

####Priority Blocking Queue [<b>Stable</b>] Version: 1.3

The priority_queue, is a simple, synchronized queue that sorts elements based on the comparator passed, if there is one. If there isn't one, then it acts a normal queue, making it flexible. 

It's enqueue and dequeue allows the use of a timeout, which a timeout of 0 allows you to poll, acting as a normal non-blocking queue as need be. It's synchronized nature allows it to sorted and cleared without the limits of a lockless queue, but lacks the performance of one, but overall it is moderately light weight and very intuitive and easy to use.

If the priority_queue is destroyed, the waiting threads will wake up and exit.

#####Code Sample

```c

/// The comparator. Simple, as it just compares two integers.
int compare_vals(void *arg_one, void *arg_two) {
    return *(int *)arg_one - *(int *)arg_two;
}

/// Create the queue. If the max size is 0, it is unbounded.
priority_queue_t *queue = priority_queue_create(0, compare_vals);

int num = 1;
/// Enqueue's timeout does nothing if it is unbounded as it will never block.
priority_queue_enqueue(queue, &num, -1);

/// Dequeue on the other handle will block if it is empty.
priority_queue_dequeue(queue, -1);
/// Now forcefully dequeue until timeout of 5 seconds, as it is now empty.
priority_queue_dequeue(queue, 5);
/// Now, purposefully wait undefinitely, normally this will cause a deadlock if no other thread enqueues, but observe.
priority_queue_dequeue(queue, -1);

/// Now imagine this is called in another thread...
priority_queue_destroy(queue, free);
/*
    priority_queue_destroy takes a callback which fits free perfectly, but any 
    other function can be used. If the queue has threads waiting on it, like 
    MU_Events, it will wake up all threads and wait for it to exit 
    appropriately before destruction.
*/
```

####Vector [<b>Unimplemented</b>]

#####Planned

* Simple vector implementation using arrays.
    - Synchronized with spinlock.

####Lockless Stack [<b>Unstable</b>]

The lockless stack utilizes MU_Hazard_Pointers to avoid the ABA problem and allow safe deallocation of nodes after they are popped off the stack. The stack is guaranteed not to lock, hence all threads are constantly making progres, and will yield if they fail on atomic compare and swaps to lower contention. 

#####Code Sample

```c

stack_t *stack = stack_create();
stack_push(stack, "Hello World");
stack_pop(stack);
stack_destroy(stack, free);

```

####Lockless Queue [<b>Unstable</b>]

The lockless queue utilitizes hazard pointers to solve the ABA problem, is fast and minimal, and guaranteed to never block or deadlock. Like the stack, it will yield if it fails on a compare and swap to lower overall contention

#####Code Sample

```c

queue_t *queue = queue_create();
queue_enqueue(queue, "Hello World");
queue_dequeue(queue);
queue_destroy(queue, free);

```

####Ring Buffer [<b>Unimplemented</b>]

#####Planned

* Lock-Free Ring Buffer implementation
* Allows writing and read
    - Writing rings around, overwriting once full

####Hash Map [<b>Stable</b>] Version: 1.0

A basic, synchronized hash map implementation. It's thread-safe, but not lockless, yet it fulfills it's purpose. It takes string keys, but it's value can be anything. 

#####Code Sample

```c

const int init_bucket_size = 31;
const bool synchronized = true;

// Assume it just returns the string directly.
char *to_string(void *data);

map_t *map = map_create(init_bucket_size, synchronized);

map_add("Hello World", "How are you");
printf("%s", map_get("Hello World"));

size_t key_val_size;
char **key_val_pairs = map_key_value_to_string(
    map, "(", ",", ")",&key_val_size, to_string
);

```

Overall, it's simple and intuitive to use.

####Deque [<b>Unimplemented</b>]

#####Planned

* Double-edged Queue
    - Pop and Dequeue
    - Push and Enqueue
* Spinlock for synchronization

###Misc. Utilities

####Timer [<b>Unstable</b>] Version: 1.0

A basic timer utility, allowing you to start and stop a timer and get a string representation of the total time.

#####Code Sample

N/A

####Flags [<b>Stable</b>] Version: 1.0

Provides extremely simple yet extremely useful flags for bitmasking. In fact, it is so simple, you actually do not even need to know how bitwise operations even work. They provide macros that allow you to determine if a flag is set in a mask, to set a flag, clear a flag or even toggle a flag. They are extremely simple, once again.

#####Code Sample

```c

/// Sample flag, the easiest way to bitwise flags without doing the math yourself
#define SIMPLE_FLAG 1 << 0
/// You can also use a constant expression.
static const unsigned int scope_respecting_flag = 1 << 1;
/// Or use an enumeration
typedef enum {
    flag_one = 1 << 2,
    flag_two = 1 << 3,
    flag_three = 1 << 4
} flags;

/// Now to show how to use the given MU_Flag macros.

/// Initialize a mask of flags.
unsigned int mask = SIMPLE_FLAG | scope_respecting_flag | flag_one;
/// Can determine if a flag has been passed in the mask above. Will be true.
bool has_simple_flag = MU_FLAG_GET(mask, scope_respecting_flag);
/// Sets the flag_two flag in the mask
FLAG_SET(mask, flag_two);
/// Removes the SIMPLE_FLAG mask from the mask
FLAG_CLEAR(mask, SIMPLE_FLAG);
/// Toggles flag_three on.
FLAG_TOGGLE(mask, flag_three);

```

####Argument Checking [<b>Stable</b>] Version: 1.1

Features a very simple and easy to use macro that can check up to 8 arguments, logging the conditionals as strings and whether or not they are true or false. It should be noted that due to the limitations of macros, it does not feature short-circuit evaluations, hence if you are going to be checking struct members for validity you must check each time to see if the struct exists.

If any of the conditions fail, it will output the following. For this example, assume test's is_valid member is false.

Invalid Arguments=> { msg: TRUE; val > 0 && val < 100: TRUE; test: TRUE; test && test->is_valid: FALSE }

#####Code Sample

```c

// Assume this is initialized sometime before test_func is called.
logger_t *logger;

typedef struct {
    bool is_valid;
} test_struct;

bool test_func(char *msg, int val, test_struct *test) {
    ARG_CHECK(logger, false, msg, val > 0 && val < 100, test, test && test->is_valid);
    /*
        ARG_CHECK takes a logger to log to, the return value, and then up to 8 arguments. Once again note that you must short-circuit test to get it's is_valid member safely as this is a limitation of macros.
    */
}

```

####Allocation Checker

Simple macros which check for a bad allocation, and if so will execute the block of code after it, using the macro for loop trick. So, if you wanted to use malloc, but return NULL or free up other resources on error, you would define the on-error block which will ONLY be called if things go wrong.

To 

#####Code Sample

~~~c

static logger_t *logger;

char *str;
ON_BAD_MALLOC(str, logger, 6)
    goto err;

pthread_mutex_t *lock;
ON_BAD_CALLOC(lock, logger, sizeof(*lock))
    goto err_lock;

int *arr;
ON_BAD_REALLOC(&arr, logger, sizeof(*arr) * 5)
    goto err_arr;

err_arr:
    free(lock);
err_lock:
    free(str);
err:
    return NULL;

~~~

####Portable TEMP_FAILURE_RETRY [<b>Stable</b>] Version: 1.0

As GCC's TEMP_FAILURE_RETRY macro allows you to restart functions which return -1 and set errno to EINTR, which allow for consistent programming regardless of signals. The macro I implement is merely, an abuse of the comma operator to loop until EINTR is no longer set.

#####Code Sample

```c

FILE *file = fopen(...);
/// Assume this contains a valid file.
char buf[BUFSIZ];
size_t bytes_read;
C_UTILS_TEMP_FAILURE_RETRY(bytes_read, fread(buf, 1, BUFSIZ, file));
/// Etc.

```

####Notice

As of now, Stable is very outdated, and this is a VERY big overhaul. Hence forth, this README will look strange when facing only the stable version. Hence, I urge that you look at the development section...

https://github.com/theif519/C_Utils/tree/development
