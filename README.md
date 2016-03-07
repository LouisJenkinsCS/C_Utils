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

##Libraries Packages

Library Packages below have numerous tools and utilities for use. Sometimes, you may want to use all of them, and therefore, there will be a header for each which includes all other headers. Hence, if you want all data structures, then you may be able to include <C_Utils_Data_Structures.h>

###Threading Utilities

Provides utilities, abstractions and tools which help automate/manage multithreading,

#### Thread Pool [<b>Stable</b>] Version: 1.3

TU_Pool is a thread pool with it's own priority queue for tasks. As implied by the use of a priority queue, tasks may be submitted via 6 different priorities, Lowest, Low, Medium, High and Highest. High Priority tasks would jump ahead of tasks of Low priority, intuitively. 

The static thread pool maintains a steady amount of threads, never growing or shrinking in size, however unused threads will block, hence it will not waste resources waiting for a new task to be submitted. 

Each task can return an asynchronous result, which, based on my implementation of events, you may wait (or poll) for when the task finishes. So, to reiterate, a task, by default, returns a result which can be waited on.

When submitting tasks, it comes with it's own default priority and will return a TU_Result_t result to wait on, but by passing certain flags, like TU_HIGH_PRIORITY | TU_NO_RESULT you may flag tasks specifically.

Finally you can pause the thread pool, meaning, that currently running tasks finish up, but it will not run any more until after either a timeout elapses or the call to resume is made.

Another note to mention is that the thread pool showcases the use of MU_Events, as waiting on a result is an event, so is to pause and resume. 

An example of use of the thread pool is detailed below...

```c

/// Initialize a pool of 10 threads, enough to handle anything without wasting resources.
static const size_t pool_size = 10;

/// Example task for thread pool to run.
static void *task_example(void *args);

TP_Pool_t *tp = TP_Pool_create(pool_size);
/// Adds a task of medium priority (default), with no argument (NULL) nor flags (0)
TP_Result_t *result = TP_Pool_add(tp, task_example, NULL, 0);
/// Adds the same task of high priority, no argument, and without a result.
TP_Pool_add(tp, task_example, NULL, TP_HIGH_PRIORITY | TP_NO_RESULT);
/// Note that you do not need to worry about cleaning up the result from TP_NO_RESULT.
/// Wait on result with no timeout (-1).
void *retval = TP_Result_get(result, -1);
/// Destroy the result, as it no longer is needed.
TP_Result_destroy(result);
/// Pause the thread pool for 5 seconds.
TP_Pool_pause(tp, 5);
/// Wait on Thread Pool to finish everything.
TP_Pool_wait(tp, -1);
/// Destroy thread pool.
TP_Pool_destroy(tp);

```

Very simple to use, very powerful as all thread pools are. By passing bit flags, it makes it easier to add newer features without adding more parameters and breaking things. It's fast, efficient and doesn't leak.

Very Outdated: Documentation for version 1.1 available [here](http://theif519.github.io/Thread_Pool_Documentation/).

####Scoped Locks [<b>In Development</b>] Version: .5

An implementation of a C++-like scope_lock. The premise is that locks should be managed on it's own, and is finally made possible using GCC and Clang's compiler attributes, __cleanup__. The locks supported so far are pthread_mutex_t, pthread_spinlock_t, and sem_t. It will lock when entering the scope, and unlock when leaving (or in the case of sem_t, it will increment the count, and then decrement). This abstracts the need for the need to lock/unlock the lock, as well as generifying the type of lock used as well, as the allocation is done using C11 generics.

It is very simple as easy to use, and can even be described to as elegant. An optimistic example can be seen below...

```c

// Already setup and allocated.
pthread_mutex_t *lock;
// Create a scoped lock instance of the given mutex.
TU_Scoped_Lock_t *s_lock = TU_SCOPED_LOCK_FROM(lock);
// Demonstrates Scoped Lock
TU_SCOPED_LOCK(s_lock) {
    do_something();
    do_something_else();
    if (is_something) {
        // Note, we return without needing to unlock.
        return;
    }
    finally_do_something();
}
// As well as single line scoped locks
TU_SCOPED_LOCK(s_lock) some_short_operation();

```

That's it, and now the need to lock is abstracted completely. Note as well that we can replace pthread_mutex_t with a pthread_spinlock_t and it would work the exact same, because everything else is managed in the background.

####Conditional Locks [<b>Stable</b>] Version: 1.0

Features auto-logging locking macros for mutexes and rwlocks. It simply checks if the lock if NULL before attempting to lock, as attempting to lock a NULL pthread_*_t argument will cause a segmentation fault. Also should note that if something goes wrong, I.E on EDEADLK, it will log the precise location of said errors.

Examples of it's use are below...

```c

/// Assume this gets initialized before being called.
pthread_rwlock_t *lock;
/// Etc.
TU_COND_RWLOCK_RDLOCK(lock, logger);
/// Later, maybe in some other thread...
TU_COND_RWLOCK_WRLOCK(lock, logger);

```

Now imagine you have a data structure that uses rwlocks, or even mutexes. Now, the overhead of a mutex, no matter how optimized they are, is still unneeded on single threaded applications for said data structure. Hence, if lock is NULL it will result in a NOP, and do nothing. The compiler may even optimize away the check entirely and act like it's not there, who knows. The point being that it allows for more flexible data structures which can't be made lockless.

####Events [<b>Stable</b>] Version: 1.1

An implementation of Win32 Events. As of yet, it allows you to wait on an event, which is equivalent to waiting on a condition variable, signaled by other threads. 

TU_Events allows you to wait on events, and supports flags which allow you to set the default state, whether or not to signal the event after a timeout, and whether or not to auto-reset the event after a thread exits the event, or after the last waiting thread leaves. 

TU_Events is an abstraction on top of a pthread_mutex, pthread_cond variable, and other flags. MU_Events are entirely thread safe and efficient, and also entirely flexible, coming with it's own MU_Logger support. You can also name events and pass the thread identifier to allow debugging said events easier.

An example of it's usage can be seen below...

```c

/// Logger for events. Assume it gets initialized and setup before calling events.
struct c_utils_logger *event_logger;
/*
    The event object used for signaling and waiting on events.
    This event is named "Test Event" and logs to the event logger,
    inituitively. It is signaled by default, hence those calling to wait on it
    will return immediately. The first thread to leave this event
    successfully, will reset the event to non-signaled state.
*/
TU_Event_t *event = MU_Event_create("Test Event", event_logger, TU_EVENT_SIGNALED_BY_DEFAULT | TU_EVENT_AUTO_RESET);
/// We now to want wait on this event. Thread identifier can be anything, but lets just use pthread_self.
TU_Event_wait(event, -1, (unsigned int) pthread_self());
/// Now some other thread signals this...
TU_Event_signal(event, (unsigned int) pthread_self());
/* 
    Now, we're done with said event. Destroy it. Note that if any threads are 
    waiting on it, they are woken up and can gracefully exit.
*/
TU_Event_destroy(event, (unsigned int) pthread_self());

```

Once again, simple and easy to use. The thread_id is to help with debugging, as it is unfortunately impossible to get an actual workable number to work with for a thread, but it's irrelevant in the example. Assigning your own thread_id to any given thread you spawn makes debugging easier. Alternatively, you can just past 0 if you don't care. 

####Event Loop [<b>Unstable</b>] Version 0.5

TU_Event_Loop, is a simple, minimal event loop, which allows you to add event sources, and have them be polled on at regular intervals. The Event Loop also has support for timed or timer events, upon which the dispatch callback will be called when it's timeout ellapses.

The Event Loop is rather simple and bare bones for now, polling once every 10ms, hence the amount of precision is very high, yet somewhat expensive, however not too much so, but does not scale well when idle and does better when it has a lot of tasks to do/poll for.

The Event Loop takes a prepare callback (to prepare any such data to be passed to an event when ready), a check callback (to check if the event is ready), a dispatch callback (to notify any threads waiting on the event), and finally a finalize callback (to destroy the user data when it is finished).

An example of it's completed state (optimistically) will look somewhat akin to this...

```c

void *prepare_event(void *data) {
    return malloc(sizeof(struct some_event_t));
}

bool check_event(void *data) {
    if (do_something_with(data)) return true;
    return false;
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
    TU_Event_Source_t *event = TU_Event_Source_create(prepare_event, check_event, dispatch_event, finalize_event, 0);
    TU_Event_Source_t *timed_event = TU_Event_Source_create(NULL, NULL, print_something, NULL, 10);
    TU_Event_Loop_t *loop = TU_Event_Loop_create();
    TU_Event_Loop_add(loop, event);
    TU_Event_Loop_add(loop, timed_event);
    TU_Event_Loop_run(loop);
    return 0;
}

```

It's very simple, and as always, easy to use. Right now it's not entirely stable, but I'm working on fixing bugs and adding appropriate features.

###Memory Management Utilities

Memory Management Utilities provide useful tools and abstractions which will help automate, or at the very least, improve quality-of-life for a C programmers, which can be helpful to an amateur or a professional. All of the below are thread safe.

####Hazard Pointers [<b>Unstable</b>]

Provides a flexible and easy to use implementation of hazard pointers, described in the research paper by Maged M. Michael, [here](http://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf).

The implementation is still in development and needing of testing, however I have an optimistic outlook on how it will look. Basically, you will "acquire" data via it's pointer and "release" it when you are finished. You must make sure to release the reference once finished with it, through the API calls MU_Hazard_Pointer_acquire() and MU_Hazard_Pointer_release and MU_Hazard_Pointer_release_all.

Another notable feature is that you do not need to keep your own reference to the hazard pointer itself, as it's allocated as thread-local storage and allocated on first use. Lastly, any remaining data not freed before the program ends, will be destroyed when the library is unlinked (I.E program termination).

An optimistic example of it's finished API looks like this...

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
    if (!head) return NULL;
    MMU_Hazard_Pointer_acquire(head);
    if (head != stack->head) {
        MMU_Hazard_Pointer_release(head);
        usleep(250);
        continue;
    }
    next = head->next;
    if (__sync_bool_compare_and_swap(&stack->head, head, next)) break;
    MMU_Hazard_Pointer_release(head);
    usleep(250);
}
void *data = head->item;
// true = retire data, set to be deleted later, false = just remove reference
MMU_Hazard_Pointer_release(head, true);

```

That's it. The above is a snippet of the lockfree stack I created using it. Note that each time you, must release reference to the pointer, and usleep is used to lower overall contention to prevent threashing of the CPU.

Overall, it's rather simple.

####Reference Counting Memory Allocator [<b>Unimplemented</b>]

####Object Pool [<b>Unimplemented</b>]

###String Utilities

Supplies basic string manipulations that are intuitive and easy to use.

####String Manipulations [<b>Stable</b>] Version: 2.0

A basic, yet very powerful and conventional string manipulations library. Supports ASCII strings only, and some functions support the use of non NULL-terminated functions.

From simple string reversal or splitting and joining a string based on a delimiter, or even dynamic concatenation of strings, is all included in this library. This library fixes and improves upon the standard libc and glibc library by adding functionality that is sorely missing, in an efficient manner.

There is also a convenience typedef for cstrings, String, which abstracts the need to use pointers. Lastly, there is a convenience macro that can be used to handle memory management of non-constant strings, TEMP, which utilitizes the GCC or Clang's compiler attributes.

Examples of it's use can be seen below...

```c

/*
    The below demonstrates the ease of use of declaring a string with the
    typedef provided. Alternatively, you can declare it as char *str, which
    can be used interchangeably.
*/
String str = "Hello World";
/*
    The below demonstrates the memory management of strings being handled by
    the compiler, automatically being destroyed when it leaves the scope of the
    block of code.
*/
String TEMP str = strdup("Hello World");
/*
    Now, on to the actual functions of this string manipulation library.
    First, we will attempt to reverse a portion of the string, str, declared
    above. We only wish to reverse "World" however, so we will make use of
    pointer arithmetic to get the offset of the string. We want to reverse
    everything after after "Hello ", so we pass 0 as the length to specify
    that it is null terminated and that strlen can be used.
*/
SU_String_reverse(str + 6, 0);
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
String storage;
SU_STRING_CONCAT_ALL(&storage, ",", str, "How are you today", "Good I hope", "Good day!");
/*
    The other functions are rather straight forward, however to go more into 
    SU_STRING_CONCAT_ALL, notice you do not need to add a NULL dummy parameter
    or specify the size. That's because the preprocessor can determine it for
    you, which it does.
*/
```

SU_String is rather powerful, and also very simple. The String typedef makes it easier for programmers coming from other languages to read, the TEMP modifier helps with memory management, and the string manipulation functions are rather intuitive and easy to use. 

OUTDATED: Documentation for version 1.2 available [here](http://theif519.github.io/String_Utils_Documentation/).

####String Buffer [<b>In Development</b>]

Abstracts away the need to manually allocate strings and do tedious string manipulations. The string_buffer automatically manages resizing itself and shrinking when needed. It features a generic macro (requires C11 _Generic keyword) to automatically append, prepend or insert any of the standard types. It is also optionally thread-safe.

Examples of usage below...

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

Very simple. The string_buffer supports an option to enable synchronizaiton, which is done through a spinlock. Majority of cases do not require synchronization, however if ever you have a case where you require one, for say a producer-consumer relationship, it can be enabled easily. 

####Regular Expressions [<b>Unimplemented</b>]

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

####File Buffering [<b>Unimplemented</b>]

####Asyncronous Streams [<b>Unimplemented</b>]

###Networking Utilities

Provides basic networking utilities which allow a developer with almost no experience with sockets to manage connections, send/receive data, etc. Also features two managers and recycling pools for connections, allowing for efficient use.

####Connection [<b>Stable</b>] Version: 1.0

NU_Connection is the base file for all transactions on sockets and between endpoints. It contains all sending and receiving functions and logs everything. It can be created manually through NU_Connection_create() and NU_Connection_init() but it's best created through the abstractions, NU_Server and NU_Client. 

To send and receive data (as well as files) is rather simple. The examples below assume that the connection has been correctly configured and set up, most likely through NU_Server and NU_Client...

```c

/// Unlike normal bsd socket functions, my abstractions provide a timeout.
const int timeout = 60;
/// The flags to be passed to send().
const int send_flags = 0;
/// The flags to be passed to recv().
const int recv_flags = 0;

NU_Connection_t *conn;
/// Assume it's been setup and configured and already.
char buf[BUFSIZ];
size_t bytes_sent = NU_Connection_send(conn, buf, BUFSIZ, timeout, send_flags);
assert(bytes_sent);
// Simple error checking, assertions are easy for small programs.
size_t bytes_received = NU_Connection_receive(conn, buf, BUFSIZ, timeout, recv_flags);
assert(bytes_sent);
printf("%.*s", (int)bytes_received, buf);

```

Now wrap the function calls in a while loop and get user input to send, and you've got a simple server-client chat. It's rather simple, yet powerful, as all BSD socket applications are.

####Server [<b>Stable</b>] Version: 1.0

The philosophy of the NU_Server is that it acts as a manager for NU_Connection objects, which are completely configured and connected to a client. The server also manages a resource and recycling pool of NU_Connection objects as well for after you disconnect them. The server also allows you to manage multiple bindings of ports. 

To create a connection through NU_Server, see the following example...

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

NU_Server_t *server = NU_Server_create(connection_pool, bound_socket_pool, is_threaded);
/// connection_pool used as backlog too.
NU_Bound_Socket_t *bsock = NU_Server_bind(server, connection_pool, port, ip_addr);
NU_Connection_t *conn = NU_Server_accept(server, bsock, timeout);

```

And that's it. When you are finished with bsock, call NU_Server_unbind(server, bsock). When you are finished with the connection, call NU_Server_disconnect(server, conn), and it will add them back to the resource pool so they can be reused.

####Client [<b>Stable</b>] Version: 1.0

The philosophy of NU_Client follows NU_Server, in that it acts as a manager for NU_Connection objects, completely configured and initialized. Unlike the client, obviously, there is no need to bind to a port or create a bound socket type object. 

An example can be seen below.

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

NU_Client_t *client = NU_Client_create(connection_pool, is_threaded);
NU_Connection_t *conn = NU_Client_connect(client, ip_addr, port, timeout);

```

Even shorter than NU_Server is. Create the client and go. Just like NU_Server, if you are finished with a NU_Connection, call NU_Client_disconnect to add it back to the pool.

####HTTP [<b>Unstable</b>] Version 0.5

NU_HTTP, or Net_Utils's HTTP Parser, is a simple yet minimal parsing, and generating, HTTP library. It allows you to parse HTTP requests and responses, as well generate your own by setting fields, resposne statuses, etc. by the use of it's API.

NU_HTTP takes a buffer, not having to be NULL-terimainted, and returns what's left in the buffer after it parses out the rest. Hence, if you pass both the HTTP header and the message body, it will return the offset (note here) of where the message body begins. It allows you to check if a field is set by using a hash table of it's field-value pairs, file path, response status, HTTP version, etc. 

Lastly, it allows you to create an HTTP response or request in an elegant way. An example can be seen below...

```c

/// Assume header gets filled out by some request.
char header[BUFSIZ];
size_t request_size;
NU_Request_t *req = NU_Request_create();
NU_Request_append_header(req, header, &request_size);
FILE *file = fopen(req->path, "r");
NU_Response_t *res = NU_Response_create();
/*
    Note that it takes a rather elegant looking key-value pair, in the guise
    of a struct with two char * members. What the macro does, in gist, is
    that it takes (NU_Field_t) { x, y}, into { x, y} by converting it for you.
    Hence (NU_Field_t) { "Content-Length", file_size } becomes a much better:
    { "Content-Length", file_size }.
*/
NU_RESPONSE_WRITE(res, status, NU_HTTP_VER_1_0, { "Content-Length", get_page_size(file) }, { "Content-Type", content_type });
char *response = NU_Response_to_string(res);

```

It's rather simple and elegant (in the creator's biased opinion).

###Data Structures

####Iterator [<b>In Development</b>] Version: 0.5

A general-use iterator allowing generic iteration over multiple different data structures which support it's operations. It's very callback heavy, but is filled out via the data structures which create them. 

Each implementation of the iterator have node-validation, as in even though the node may have been removed from the list between uses, it will attempt to correct itself as best it can, at worst starting over at the beginning of the data structure.

Will support generic use, as in, being able to iterate through linked lists, vectors, even hash tables without having to concern over the actual data type. I.E, containing an array of iterators for different data structures, allowing polymorphic operations. 

An optimistic example of it's use when finished can be seen below...

```c

/*
    Imagine that the below data structures are initialized already, containing strings.
*/
struct c_utils_list *list;
DS_Vector_t *vec;
DS_Map_t *map;
/*
    Obtain the iterator of each in an array of iterators, like below.
*/
DS_Iterator_t it[] = { c_utils_list_iterator(list), DS_Vector_iterator(vec), DS_Map_iterator(map) };
for (int i = 0; i < 3; i++) {
    char *str;
    while (str = DS_Iterator_next(it)) puts(str);
}

```

As the iterator is still in development, very early in particular, it most likely will be subject to change. Overall, it's as close as it will get to a generic iterator, but useful in certain situations I am sure.

####Linked List [<b>Stable</b>] Version: 1.2

A simple, yet robust double linked list implementation. It is thread-safe, and with the use of read-write locks, allows for very efficient read-often-write-rarely uses, but it's also good for general usage as well. 

It features a way to sort the list through the use of comparators, a for-each callback that can be called on all items in the list, a print-all function to print everything in a neat, formatted way, and an implementation for DS_Iterator. 

It's uses are very simple, as can be shown in the example below...

```c

int comparator(void *item_one, void *item_two);

const bool synchronized = true;
void *item, *item_two;

struct c_utils_list *list = c_utils_list_create(synchronized);
// Assume item was already allocated and points to a valid piece of memory.
c_utils_list_add(list, item, NULL);
// We added the item to the list, unsorted. The third argument is a callback to add in sorted order.
c_utils_list_add(list, item_two, comparator);
// The list is "Smart" enough to keep track of if an unsorted item was added, and will sort the list for you.
c_utils_list_get(list, 0);
c_utils_list_remove(list, 1);
c_utils_list_destroy(list, free);

```

Very simple and easy.


Documentation for version 1.0 available [here](http://theif519.github.io/Linked_List_Documentation/).

####Priority Blocking Queue [<b>Stable</b>] Version: 1.3

The Priority Blocking Queue, or DS_PBQueue, is a simple, synchronized queue that sorts elements based on the comparator passed, if there is one. If there isn't one, then it acts a normal queue, making it flexible. 

It's enqueue and dequeue allows the use of a timeout, which a timeout of 0 allows you to poll, acting as a normal non-blocking queue as need be. It's synchronized nature allows it to sorted and cleared without the limits of a lockless queue, but lacks the performance of one, but overall it is moderately light weight and very intuitive and easy to use.

An example of it's use is detailed below...

```c

/// The comparator. Simple, as it just compares two integers.
int compare_vals(void *arg_one, void *arg_two) {
    return *(int *)arg_one - *(int *)arg_two;
}

/// Create the queue. If the max size is 0, it is unbounded.
DS_PBQueue_t *queue = DS_PBQueue_create(0, compare_vals);
int num = 1;
/// Enqueue's timeout does nothing if it is unbounded as it will never block.
DS_PBQueue_enqueue(queue, &num, -1);
/// Dequeue on the other handle will block if it is empty.
DS_PBQueue_dequeue(queue, -1);
/// Now forcefully dequeue until timeout of 5 seconds, as it is now empty.
DS_PBQueue_dequeue(queue, 5);
/// Now, purposefully wait undefinitely, normally this will cause a deadlock if no other thread enqueues, but observe.
DS_PBQueue_dequeue(queue, -1);
/// Now imagine this is called in another thread...
DS_PBQueue_destroy(queue, free);
/*
    DS_PBQueue_destroy takes a callback which fits free perfectly, but any 
    other function can be used. If the queue has threads waiting on it, like 
    MU_Events, it will wake up all threads and wait for it to exit 
    appropriately before destruction.
*/
```

Not only is the Priority Blocking Queue easy to use, but it also provides timeouts to prevent deadlocks and allow execution to come back to the user. And even on a thread waiting indefinitely, it will be woken up, preventing an eternal deadlock.

Very Outdated: Documentation for version 1.0 available [here](http://theif519.github.io/Data_Structures_Documentation/Priority_Blocking_Queue/).

####Vector [<b>Unimplemented</b>]

####Lockless Stack [<b>Unstable</b>]

The lockless stack utilizes MU_Hazard_Pointers to avoid the ABA problem and allow safe deallocation of nodes after they are popped off the stack. The stack is guaranteed not to lock, hence all threads are constantly making progres, and will yield if they fail on atomic compare and swaps to lower contention. 

Overall, it's a simple, yet powerful thread-safe, lockless data structure.

```c

DS_Stack_t *stack = DS_Stack_create();
DS_Stack_push(stack, "Hello World");
DS_Stack_pop(stack);
DS_Stack_destroy(stack, free);

```

That's all there is to it.

####Lockless Queue [<b>Unstable</b>]

The lockless queue utilitizes hazard pointers to solve the ABA problem, is fast and minimal, and guaranteed to never block or deadlock. Like the stack, it will yield if it fails on a compare and swap to lower overall contention

Also like the stack, it is a very simple, yet powerful thread-safe, lockless data structure.

```c

DS_Queue_t *queue = DS_Queue_create();
DS_Queue_enqueue(queue, "Hello World");
DS_Queue_dequeue(queue);
DS_Queue_destroy(queue, free);

```

That's all there is to it.

####Ring Buffer [<b>Unimplemented</b>]

####Hash Map [<b>Stable</b>] Version: 1.0

A basic, synchronized hash map implementation. It's thread-safe, but not lockless, yet it fulfills it's purpose. It takes string keys, but it's value can be anything. 

As a hash table isn't anything special, I'll give a simple demonstration of it's use below.

```c

const int init_bucket_size = 31;
const bool synchronized = true;

// Assume it just returns the string directly.
char *to_string(void *data);

DS_Map_t *map = DS_Map_create(init_bucket_size, synchronized);
DS_Map_add("Hello World", "How are you");
printf("%s", DS_Map_get("Hello World"));
size_t key_val_size;
char **key_val_pairs = DS_Map_key_value_to_string(map, "(", ",", ")",&key_val_size, to_string);

```

Overall, it's simple and intuitive to use.

####Deque [<b>Unimplemented</b>]

###Misc. Utilities

####Timer [<b>Unstable</b>] Version: 1.0

A basic timer utility, allowing you to start and stop a timer and get a string representation of the total time.

####Flags [<b>Stable</b>] Version: 1.0

Provides extremely simple yet extremely useful flags for bitmasking. In fact, it is so simple, you actually do not even need to know how bitwise operations even work. They provide macros that allow you to determine if a flag is set in a mask, to set a flag, clear a flag or even toggle a flag. They are extremely simple, once again.

An example of the usage of MU_Flags are below...

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
MU_FLAG_SET(mask, flag_two);
/// Removes the SIMPLE_FLAG mask from the mask
MU_FLAG_CLEAR(mask, SIMPLE_FLAG);
/// Toggles flag_three on.
MU_FLAG_TOGGLE(mask, flag_three);

```

Just like everything else, extremely simple. They compress 5 different flags into one unsigned integer, making it very flexible when adding more. The amount of flags is dependent on the type of flag. A 16-bit short can handle 16 flags, while an integer can handle 32, while a long can handle 64 flags. Very intuitive, and easy to use.

####Argument Checking [<b>Stable</b>] Version: 1.1

Features a very simple and easy to use macro that can check up to 8 arguments, logging the conditionals as strings and whether or not they are true or false. It should be noted that due to the limitations of macros, it does not feature short-circuit evaluations, hence if you are going to be checking struct members for validity you must check each time to see if the struct exists.

An example of it's use can be seen below.

```c

// Assume this is initialized sometime before test_func is called.
struct c_utils_logger *logger;

typedef struct {
    bool is_valid;
} test_struct;

bool test_func(char *msg, int val, test_struct *test) {
    MU_ARG_CHECK(logger, false, msg, val > 0 && val < 100, test, test && test->is_valid);
    /*
        MU_ARG_CHECK takes a logger to log to, the return value, and then up to 8 arguments. Once again note that you must short-circuit test to get it's is_valid member safely as this is a limitation of macros.
    */
}

```

If any of the conditions fail, it will output the following. For this example, assume test's is_valid member is false.

Invalid Arguments=> { msg: TRUE; val > 0 && val < 100: TRUE; test: TRUE; test && test->is_valid: FALSE }

It's very simple yet very easy to do in each function. If you have more than 8 arguments you can do more than one MU_ARG_CHECK as there are amount of arguments / 8.

####Portable TEMP_FAILURE_RETRY [<b>Stable</b>] Version: 1.0

As GCC's TEMP_FAILURE_RETRY macro allows you to restart functions which return -1 and set errno to EINTR, which allow for consistent programming regardless of signals. The macro I implement is merely, an abuse of the comma operator to loop until EINTR is no longer set. It essentially is the below...

```c

#define MU_TEMP_FAILURE_RETRY(storage, function) while (errno = 0, storage = function, errno = EINTR)

```

It's use can be noted below...

```c

FILE *file = fopen(...);
/// Assume this contains a valid file.
char buf[BUFSIZ];
size_t bytes_read;
MU_TEMP_FAILURE_RETRY(bytes_read, fread(buf, 1, BUFSIZ, file));
/// Etc.

```

It isn't as fluent as GNU's gcc macro, as I'm unsure how it is implement as it allows the return of the function call, but mine stores it inside of what's passed in storage argument.

####Notice

Documentation will be split between the two later, as originally they both were contained within one file. However, the original documentation can be found below!

Documentation is also outdated, but will be updated eventually once everything is stable.

Documentation for version 1.0 available [here](http://theif519.github.io/Misc_Utils_Documentation/).

## Notes

Not all packages here are finished, neither do the list of packages here reflect the finished amount.
