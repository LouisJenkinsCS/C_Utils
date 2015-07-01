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

### String Utils [<b>Unstable</b>] -- All but from_token work, which produces a memory leak.

A basic, non-intrusive and well-tested string manipulations library as well as parsing strings with regular expressions (future).

Documentation for version 1.2 available [here](http://theif519.github.io/String_Utils_Documentation/).

### File Utils [<b>Unimplemented</b>]

### Networking Utils [<b>In Development</b>]

####Server [<b>In Development</b>]

Easily create a server with this wonderful abstraction on top of BSD sockets, on however many ports you want, with however many clients you want! Supports sending and receiving data such as messages as files.

####Client [<b>In Development</b>]

Easily connect to already-established servers, fully capable of sending and receiving data, as well as files.

####HTTP [<b>Unimplemented</b>]

Easily handle HTTP requests or make your own with this abstraction on top of the Client-Server mentioned above.

### Data Structures [<b>In Development</b>]

####Linked List [<b>Stable</b>]

A generic and general use Linked List utilizing void pointers and callbacks. Sort elements, Iterate through it, and construct and deconstruct them from/to arrays! If you need a dynamic storage of elements that's thread-safe and without a real worry for optimal performance, then this is the best for you.

Documentation for version 1.0 available [here](http://theif519.github.io/Linked_List_Documentation/).

####Priority Blocking Queue [<b>Stable</b>]

Also known as PBQueue in my library, is a data structure which allows you to enqueue and dequeue elements, in a sorted order, and blocks up to the timeout or if the operation can proceed. Naturally thread-safe.

Documentation for version 1.0 available [here](http://theif519.github.io/Data_Structures_Documentation/Priority_Blocking_Queue/).

####Ring Buffer [<b>Unimplemented</b>]

####Hash Map [<b>Unimplemented</b>]

####Deque [<b>Unimplemented</b>]

### Misc Utils [<b>In Development</b>]

####Logger [<b>Stable</b>]

A basic logging utility with support for logging based on levels.

####Timer [<b>Unstable</b>] -- Timer doesn't make any checks for whether it's already running or not.

A basic timer utility, allowing you to start and stop a timer and get a string representation of the total time.

####Notice

Documentation will be split between the two later, as originally they both were contained within one file. However, the original documentation can be found below!

Documentation for version 1.0 available [here](http://theif519.github.io/Misc_Utils_Documentation/).

## Notes

Not all packages here are finished, neither do the list of packages here reflect the finished amount.
