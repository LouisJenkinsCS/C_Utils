# Data Structures

##Summary

Some simple and straight-forward, but thread-safe implementations of data structures. Some, but not all, are lock-free implementations, or as lock-free as you can get without using a custom memory allocator. Lock-Free implementations utilize my Hazard Pointer implementations in Misc_Utils.

All data structures begin with the namespace 'DS_', and maintain a consistent naming convention of having a one-to-two description name, capitalized, 'c_utils_list_' or 'DS_Stack', and verb describing it's actions, I.E 'c_utils_list_create' or 'DS_Map_add', which is in lowercase to contrast with the namespace and descriptor .

##General Non-STL Dependencies

All data structures use the following for dependencies, with any else listed for each data structure. Any with the 'DS_' or 'MU_'namespace are supplied in this package.

* DS_Helpers <= Helper callback and Node declarations
* MU_Logger <= Logging errors and warnings.
* MU_Arg_Check <= Log and return on bad parameter arguments.
* MU_Cond_Locks <= Conditional synchronization and logging of threading errors.
* Pthreads <= For Read-Write locks used in synchronization.

##Double Linked List

###Features

* Optional safe concurrent access & thread safety.
* General-Use via Callbacks.
* Relatively lightweight.
* Built-in Iterator support.
* Basic sorting.

##Hash Map

###Features

* Optional Synchronization & Thread Safety.
* User-specified amount of buckets to fine-tune performance.
* Relatively lightweight.
* String keys, any values

##Priority Blocking Queue

###Features

* Synchronization & Thread Safety.
* Sorted based on comparator used.
* Relatively lightweight
* Can be bounded or unbounded.
* Blocks thread until Ready or Timeout specified.
* Blocked threads wake up when shutdown.

##Lock-Free Stack

###Features

* Lock-Free.
* Avoids ABA problem
    - Hazard Pointers
* Will not block.
* Very lightweight and fast.
* Deadlock and Priority Inversion free
* Lowered Contention.

##Lock-Free Queue

###Features

* Lock-Free
* Avoids ABA problem
    - Hazard Pointers
* Will not block
* Very lightweight and fast
* Deadlock and Priority Inversion free
* Lowered Contention

##Iterator

###Features

* General-use iterator
* Type-Agnostic
    - Traverse different kinds of data structures with support.
* Easy to use.

###Extra Non-STL Dependencies

* MU_Hazard_Pointers <= Ensures safe deletion of nodes, atomically.
* GCC and Clang compiler attributes <= For __sync_bool_compare_and_swap(...)
