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

### Thread Pool [<b>Stable</b>]

A full-featured thread pool giving the option to add prioritized tasks, pausing and resuming tasks, and obtaining results from said tasks all with moderate speed and size.

Choose between a static amount of threads or dynamically changing ones (future).

Documentation for version 1.1 available [here](http://theif519.github.io/Thread_Pool_Documentation/).

### String Utils [<b>Stable</b>]

A basic, non-intrusive and well-tested string manipulations library as well as parsing strings with regular expressions (future).

Documentation for version 1.2 available [here](http://theif519.github.io/String_Utils_Documentation/).

### File Utils [<b>Unimplemented</b>]

### Networking Utils [<b>In Development</b>]

####Connection [<b>Stable</b>]

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

####Server [<b>Stable</b>]

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

####Client [<b>Stable</b>]

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

####Linked List [<b>Stable</b>]

A generic and general use Linked List utilizing void pointers and callbacks. Sort elements, Iterate through it, and construct and deconstruct them from/to arrays! If you need a dynamic storage of elements that's thread-safe and without a real worry for optimal performance, then this is the best for you.

Documentation for version 1.0 available [here](http://theif519.github.io/Linked_List_Documentation/).

####Priority Blocking Queue [<b>Stable</b>]

Also known as PBQueue in my library, is a data structure which allows you to enqueue and dequeue elements, in a sorted order, and blocks up to the timeout or if the operation can proceed. Naturally thread-safe.

Documentation for version 1.0 available [here](http://theif519.github.io/Data_Structures_Documentation/Priority_Blocking_Queue/).

####Vector [<b>Unimplemented</b>]

####Ring Buffer [<b>Unimplemented</b>]

####Hash Map [<b>In Development</b>]

A basic thread-safe hash map. Follows the template of <char *, void *>, where you have a string as a key, and a generic void * as a value.

####Deque [<b>Unimplemented</b>]

### Misc Utils [<b>In Development</b>]

####Logger [<b>Stable</b>]

A minimal logging utility which supports logging based on log levels, with it's own custom formatting. Also supports a custom log level with custom log label for formatting. 

Example: "%tsm [%lvl](%fle:%lno) %fnc(): \n\"%msg\"\n" 

Which is the current default, would look like such...

```c

MU_Logger_t *logger = MU_Logger_create("Test_File.txt", "w", MU_INFO);
MU_LOG_INFO(logger, "Hello World!");

```

The above would produce the following output:

9:39:32 PM [INFO](test_file:63) main():
"Hello World!"

The most notable features being that it lets you know not only the exact line number and file, but also the function it is being called from.

In the future, there will be more log formats being accepted, as well as config file support and syslog support, even a lockless ring buffer and a worker thread to manage them.

####Timer [<b>Unstable</b>]

A basic timer utility, allowing you to start and stop a timer and get a string representation of the total time.

####Argument Checking [<b>Stable</b>]

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

####Conditional Locks [<b>Stable</b>]

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

####Portable TEMP_FAILURE_RETRY

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
