# Threading Utilities

## Summary

Threading Utilities provides and offers tools, utilities, and abstractions to improve quality-of-life and automate portions of multithreaded code.

## thread_pool

### Internal Dependencies

* io/logger
* data_structures/priority_queue
* threading/events
* misc/flags
* misc/argument_checking

### External Dependencies

* pthread
* C11 (stdatomic)

### Features

* Asynchronously submit tasks and (optionally) retrieve the result.
* Prioritize tasking using priority_queue.
* Pause and Resume tasks, by timeout or on-demand.
* Wait until all tasks complete or until timeout

## cond_locks

### Internal Dependencies

* io/logger

### External Dependencies

* pthread

### Features

* Optionally allocate and initialize locks
* Optionally lock/unlock mutex based on if it is allocated
* Allows an easy way to enable and disable synchronization.
* Supports...
    - pthread_rwlock_t
    - pthread_mutex_t
    - pthread_spinlock_t
* Logs the location it is called if something goes wrong
    - I.E EDEADLK

## scoped_lock

### Internal Dependencies

* io/logger

### External Dependencies

* pthread
* GCC or Clang and __cleanup__ attribute
* C11 (_Generic)

### Features

* Automatically lock and unlock when entering and exiting scope of block (respectively)
* Logs where and when something goes wrong
    - I.E EDEADLK
* If lock is null, will not lock
    - Allows for cond_locks plugin replacement
    - Easily enable and disable synchronization.

## events

### Internal Dependencies

* io/logger
* misc/flags

### External Dependencies

* pthread
* C11 (stdatomic)

### Features

* Wait on or signal events
    - Threads waiting will wait until a signal is sent
* Smart destruction
    - When destroyed, the event wakes all threads and waits for all threads to exit before destroying itself.
* Configurable
    - Contains flags which alter it's functionality
        + Resetting event when one thread exits
            * Or all threads exit
        + Event signaled by default
        + Etc.
* Logs to in a unique and distinguishable log format for each event.
    - Makes keeping an event log all the easier.

## event_loop

### Internal Dependencies

* data_structures/list
* threading/events
* misc/flags
* misc/argument_checking

### External Dependencies

* pthread
* sys/time

### Features

* Create your own events either based on a timer, or a condition, or both
    - If it does not have a timer, it will be polled on each iteration of the loop
