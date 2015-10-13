# Net_Utils

## Summary

Implements some rather simple yet powerful tools and abstractions for networking. As of yet, they include a useful BSD abstraction which encapsulates and maintains itself behind the scenes, so you only have to worry about what you want to send/receive, not how you send/receive it. Then to simplify it's use even more, there is an even more useful yet efficient manager of connections. Lastly, there also is an minimal HTTP parser.

## General Non-STL dependencies

Almost all packages use the following dependencies, available in other libraries in this very package.

* NU_Helpers <= For header files.
* MU_Logger <= For logging.
* MU_Cond_Locks <= For optional synchronization.
* Pthreads <= For locks.
* sys/* & netdb <= For networking, POSIX specific.

## Connection

### Features

* BSD Socket Abstraction.
* Send/Receive data and files to a connected end-point.
* R/W thread-safe access.
* Verbose Logging.

## Server

### Features

* BSD Socket Abstraction
* Manage and create new Connections
* Manage and bind sockets to ports.
* Synchronized thread-safe access.
* Verbose logging.

## Client

### Features

* BSD Socket Abstraction
* Manage and create new Connections
* Synchronized thread-safe access.
* Verbose logging.

## HTTP

### Features

* Simple HTTP parsing
    - Mapped
    - Response and Request
* Generate HTTP headers