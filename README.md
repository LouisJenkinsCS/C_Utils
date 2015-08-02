# Utilities Package for the C Programming Language

##Summary

Utilities Package for the C Programming Language, or C_Utils for short, is my attempt at providing a full fledged package of various libraries ranging from thread-safe data structures, to a Thread Pool to even an HTTP library! 

##About the Author

I am a Computer Science student that declared late and who hasn't really found his passion for programming until half way through Junior Year. This is my attempt at teaching myself the various needed topics all comptuer science students should know without having to wait too long to learn what should already be known. Please do not be discouraged and think that this will be an awful project, as I am committed to trying my best at completing it as best I can. 

##Notes

[<b>Unimplemented</b>] means it has not been started at all but I am planning on implementing at a later date.

[<b>In Development</b>] means that it is currently in development and should be finished soon.

[<b>Unstable</b>] means I'm not sure if it's stable, but it's usable although not well tested.

[<b>Stable</b>] means that it is stable enough to use, although features may be added later.

[<b>Finished</b>] means that in all likely hood, I'll no longer work on it unless a bug presents itself.

##Libraries available

### Thread Pool [<b>In Development</b>]

####Static Pool [<b>Stable</b>] Version: 1.2

TP_Pool is a thread pool with it's own priority queue for tasks. As implied by the use of a priority queue, tasks may be submitted via 6 different priorities, Lowest, Low, Medium, High and Highest. High Priority tasks would jump ahead of tasks of Low priority, intuitively. 

The static thread pool maintains a steady amount of threads, never growing or shrinking in size, however unused threads will block, hence it will not waste resources waiting for a new task to be submitted. 

Each task can return an asynchronous result, which, based on my implementation of events, you may wait (or poll) for when the task finishes. So, to reiterate, a task, by default, returns a result which can be waited on.

When submitting tasks, it comes with it's own default priority and will return a TP_Result_t result to wait on, but by passing certain flags, like TP_HIGH_PRIORITY | TP_NO_RESULT you may flag tasks specifically.

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

####Dynamic Pool [<b>Unimplemented</b>]

### String Utils [<b>Stable</b>] Version: 1.2

A basic, non-intrusive and well-tested string manipulations library as well as parsing strings with regular expressions (future).

Documentation for version 1.2 available [here](http://theif519.github.io/String_Utils_Documentation/).

### File Utils [<b>Unimplemented</b>]

### Networking Utils [<b>In Development</b>]

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

####HTTP [<b>In Development</b>]

As NU_HTTP is currently in development, I'll state the long term philosophy and a general demo of what it should look like as an end result.

Firstly, NU_HTTP is a simple HTTP parser, which takes a buffer, and if it finds any header fields (I.E fields which end in /r/n), it will parse out the fields and attributes into a hash map, making it easier to retrieve at a later time. Basically, it takes in a header, and spits out the rest of the message (I.E body of the message) as well as updating to you the new length of the message. This also allows it to parse incomplete headers, as if you combine the incomplete header with the rest of the header and body, it will eventually be able to finish parsing it.

After parsing the header, it can return the header as a string according to the w3 standards. It allows you to retrieve individual fields from the header files as well as set them yourself.

An optimistic example would be the following...

```c

char[BUFSIZ] request;
size_t request_size;
/// Imagine it gets filled out by NU_Connection at some time before the following calls.
NU_Request_t req;
char *left_overs = NU_Request_init(&req, request, &request_size);
/// init not only initializes the request object, but also fills it out with applicable information.
/// Assume that no header is actually over 1kb, although it is possible, not for this example.
char *file_path = NU_Request_get_file_path(&req);
FILE *file = fopen(file_path, "r");
NU_Response_t res;
NU_Response_init(&res, NULL, NULL);
NU_Response_set_status(&res, 200);
NU_Response_set_version(&res, NU_HTTP_VER_1_X);
NU_Response_set_field(&res, "Content-Length", file_size);
/// Assume you got file_size from fstat.
char [BUFSIZ] response;
NU_Response_to_string(&res, response, BUFSIZ);

/*
    Alternatively, what you CAN do is this...
    NU_Response_append_header(&res, "HTTP/1.1 200 OK\r\nContent-Length: XYZ\r\n\r\n");
    Which of course is faster.
*/

```

Unfortunately, it's rather long winded. But it's still in development, so it's bound to improve as I go along. Overall, it should allow you to set and get header fields.

### Data Structures [<b>In Development</b>]

####Linked List [<b>Stable</b>] Version: 1.1

A generic and general use Linked List utilizing void pointers and callbacks. Sort elements, Iterate through it, and construct and deconstruct them from/to arrays! If you need a dynamic storage of elements that's thread-safe and without a real worry for optimal performance, then this is the best for you.

Documentation for version 1.0 available [here](http://theif519.github.io/Linked_List_Documentation/).

####Priority Blocking Queue [<b>Stable</b>]

The Priority Blocking Queue, or DS_PBQueue, is a simple, synchronized queue that sorts elements based on the comparator passed, if there is one. If there isn't one, then it acts a normal queue, making it flexible. 

It's enqueue and dequeue allows the use of a timeout, which a timeout of 0 allows you to poll, acting as a normal non-blocking queue as need be. It's synchronized nature allows it to sorted and cleared without the limits of a lockless queue, but lacks the performance of one, but overall it is moderately light weight and very intuitive and easy to use.

An example of it's use is detailed below...

```c

/// The comparator. Simple, as it just compares two integers.
int compare_vals(void *arg_one, void *arg_two){
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
    DS_PBQueue_destroy takes a callback which fits free perfectly, but any other function can be used. If the queue has threads waiting on it, like MU_Events, it will wake up all threads and wait for it to exit appropriately before destruction.
*/
```

Not only is the Priority Blocking Queue easy to use, but it also provides timeouts to prevent deadlocks and allow execution to come back to the user. And even on a thread waiting indefinitely, it will be woken up, preventing an eternal deadlock.

Very Outdated: Documentation for version 1.0 available [here](http://theif519.github.io/Data_Structures_Documentation/Priority_Blocking_Queue/).

####Vector [<b>Unimplemented</b>]

####Ring Buffer [<b>Unimplemented</b>]

####Hash Map [<b>In Development</b>]

A basic thread-safe hash map. Follows the template of <char *, void *>, where you have a string as a key, and a generic void * as a value.

####Deque [<b>Unimplemented</b>]

### Misc Utils [<b>Stable</b>]

####Logger [<b>Stable</b>] Version: 1.4

A minimal logging utility which supports logging based on log levels, with it's own custom formatting. Also supports a custom log level with custom log label for formatting. 

Example: "%tsm \[%lvl\](%fle:%lno) %fnc(): \n\"%msg\"\n" 

Which is the current default, would look like such...

```c

MU_Logger_t *logger = MU_Logger_create("Test_File.txt", "w", MU_INFO);
MU_LOG_INFO(logger, "Hello World!");

```

The above would produce the following output:

9:39:32 PM \[INFO\](test_file:63) main():
"Hello World!"

The most notable features being that it lets you know not only the exact line number and file, but also the function it is being called from.

In the future, there will be more log formats being accepted, as well as config file support and syslog support, even a lockless ring buffer and a worker thread to manage them.

####Timer [<b>Unstable</b>] Version: 1.0

A basic timer utility, allowing you to start and stop a timer and get a string representation of the total time.

####Events [<b>Stable</b>] Version: 1.1

An implementation of Win32 Events. As of yet, it allows you to wait on an event, which is equivalent to waiting on a condition variable, signaled by other threads. 

MU_Events allows you to wait on events, and supports flags which allow you to set the default state, whether or not to signal the event after a timeout, and whether or not to auto-reset the event after a thread exits the event, or after the last waiting thread leaves. 

MU_Events is an abstraction on top of a pthread_mutex, pthread_cond variable, and other flags. MU_Events are entirely thread safe and efficient, and also entirely flexible, coming with it's own MU_Logger support. You can also name events and pass the thread identifier to allow debugging said events easier.

An example of it's usage can be seen below...

```c

/// Logger for events. Assume it gets initialized and setup before calling events.
MU_Logger_t *event_logger;
/*
    The event object used for signaling and waiting on events.
    This event is named "Test Event" and logs to the event logger, inituitively. It is signaled by default, hence those calling to wait on it will return immediately. The first thread to leave this event successfully, will reset the event to non-signaled state.
*/
MU_Event_t *event = MU_Event_create("Test Event", event_logger, MU_EVENT_SIGNALED_BY_DEFAULT | MU_EVENT_AUTO_RESET);
/// We now to want wait on this event. Thread identifier can be anything, but lets just use pthread_self.
MU_Event_wait(event, -1, (unsigned int) pthread_self());
/// Now some other thread signals this...
MU_Event_signal(event, (unsigned int) pthread_self());
/// Now, we're done with said event. Destroy it. Note that if any threads are waiting on it, they are woken up and can gracefully exit.
MU_Event_destroy(event, (unsigned int) pthread_self());

```

Once again, simple and easy to use. The thread_id is to help with debugging, as it is unfortunately impossible to get an actual workable number to work with for a thread, but it's irrelevant in the example. Assigning your own thread_id to any given thread you spawn makes debugging easier. Alternatively, you can just past 0 if you don't care. 

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
MU_Logger_t *logger;

typedef struct {
    bool is_valid;
} test_struct;

bool test_func(char *msg, int val, test_struct *test){
    MU_ARG_CHECK(logger, false, msg, val > 0 && val < 100, test, test && test->is_valid);
    /*
        MU_ARG_CHECK takes a logger to log to, the return value, and then up to 8 arguments. Once again note that you must short-circuit test to get it's is_valid member safely as this is a limitation of macros.
    */
}

```

If any of the conditions fail, it will output the following. For this example, assume test's is_valid member is false.

Invalid Arguments=> { msg: TRUE; val > 0 && val < 100: TRUE; test: TRUE; test && test->is_valid: FALSE }

It's very simple yet very easy to do in each function. If you have more than 8 arguments you can do more than one MU_ARG_CHECK as there are amount of arguments / 8.

####Conditional Locks [<b>Stable</b>] Version: 1.0

Features auto-logging locking macros for mutexes and rwlocks. It simply checks if the lock if NULL before attempting to lock, as attempting to lock a NULL pthread_*_t argument will cause a segmentation fault. Also should note that if something goes wrong, I.E on EDEADLK, it will log the precise location of said errors.

Examples of it's use are below...

```c

/// Assume this gets initialized before being called.
pthread_rwlock_t *lock;
/// Etc.
MU_COND_RWLOCK_RDLOCK(lock, logger);
/// Later, maybe in some other thread...
MU_COND_RWLOCK_WRLOCK(lock, logger);

```

Now imagine you have a data structure that uses rwlocks, or even mutexes. Now, the overhead of a mutex, no matter how optimized they are, is still unneeded on single threaded applications for said data structure. Hence, if lock is NULL it will result in a NOP, and do nothing. The compiler may even optimize away the check entirely and act like it's not there, who knows. The point being that it allows for more flexible data structures which can't be made lockless.

####Portable TEMP_FAILURE_RETRY [<b>Stable</b>] Version: 1.0

As GCC's TEMP_FAILURE_RETRY macro allows you to restart functions which return -1 and set errno to EINTR, which allow for consistent programming regardless of signals. The macro I implement is merely, an abuse of the comma operator to loop until EINTR is no longer set. It essentially is the below...

```c

#define MU_TEMP_FAILURE_RETRY(storage, function) while(errno = 0, storage = function, errno = EINTR)

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
