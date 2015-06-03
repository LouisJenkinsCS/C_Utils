#Concurrency Utilities for the C Programming Language\

##Summary

Concurrency Utilities, or Concurrency_Utils, is a package filled with robust, well-tested libraries to make concurrent programming easier for you, the developer. This package will feature, when completed, thread-safe data structures, a thread pool to abstract the need to use threads, and more.

##Current Projects

###Thread Pool

Current version: 1.1

Documentation found [here](http://theif519.github.io/Thread_Pool_Documentation/)

####Summary

The Thread Pool in this package will allow you to, the developer, to abstract the need to handle threads and their lifetimes by leaving it up to my thread pool implementation. Unlike other thread pools available for C, this one offers the ability to pause/resume and obtain the result from a task submitted. 

####Features

#####Get Results!

This thread pool returns a Result struct which allows you to wait on the returned value from a structure. Also, in the future this feature will be optional.

#####Pause all tasks to be resumed later!

Pause all tasks to be resumed later. Tasks can also be flagged to not be paused, hence the thread processing it will pause only after it finishes.

#####Prioritize your tasks!

Allows you to submit a higher priority task ahead of the tasks of lower priority.

#####Dynamic parameters based on the flags you want!

Use of bitmasking to pass multiple flags as a parameter, with it's own default behavior if you exclude them

#####Recovery from segmentation faults and errors (Future)

The Thread Pool can continue operation and just terminate the current thread, spawning a new one to take it's place, either resuming other tasks or abort all tasks and cleanup at the user's choice.

#####Static or Dynamic thread pool! (Future)

The Thread Pool allows you to use either static or dynamic thread pool constructor. With the dynamic thread pool, it allows you to pick the minimum, maximum, and average amount of threads that should be alive at any given time.

#####Multiple Thread Pools! (Future)

The Thread Pool is managed by a static virtual table, which keeps track of all currently allocated thread pools, allows for more than one thread pool at any given time, maintaining all of it's features.

####Notes and Disclaimers

There can, safely, only be one instance of this thread pool, as it's thread pool struct is a static variable.

##Future Projects

###Thread-Safe Queue, Linked List, Stack, and Hash Map

####Summary

Available at a later date (hopefully).
