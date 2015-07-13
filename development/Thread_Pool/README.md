#Concurrency Utilities for the C Programming Language\

##Summary

Concurrency Utilities, or Concurrency_Utils, is a package filled with robust, well-tested libraries to make concurrent programming easier for you, the developer. This package will feature, when completed, thread-safe data structures, a thread pool to abstract the need to use threads, and more.

##Current Projects

###Thread Pool

Current version: 1.0

Documentation found [here](http://theif519.github.io/Thread_Pool_Documentation/)

####Summary

The Thread Pool in this package will allow you to, the developer, to abstract the need to handle threads and their lifetimes by leaving it up to my thread pool implementation. Unlike other thread pools available for C, this one offers the ability to pause/resume and obtain the result from a task submitted. 

####Features

#####Get Results!

This thread pool returns a Result struct which allows you to wait on the returned value from a structure. Also, in the future this feature will be optional.

#####Pause all tasks to be resumed later!

This thread pool also features a way to pause the current tasks in it's tracks, safely, and then resume at a later time. Note that while it may safely pause and resume, this may lead to undefined behavior if you are doing critical operations, as in the futrue there will be a feature to prevent this.

#####Prioritize your tasks! (Future)

Allows you to submit a higher priority task ahead of the tasks of lower priority.

#####Recovery from segmentation faults and errors (Future)

The Thread Pool can continue operation and just terminate the current thread, spawning a new one to take it's place, either resuming other tasks or abort all tasks and cleanup at the user's choice.

####Notes and Disclaimers

There can, safely, only be one instance of this thread pool, as it's thread pool struct is a static variable.

##Future Projects

###Thread-Safe Queue, Linked List, Stack, and Hash Map

####Summary

Available at a later date (hopefully).
