# Utilities Package for the C Programming Language

##Summary

Utilities Package for the C Programming Language, or C_Utils for short, is my attempt at providing a full fledged package of various libraries ranging from thread-safe data structures, to a Thread Pool to even an HTTP library! 

##About the Author

I am a Computer Science student that declared late and who hasn't really found his passion for programming until half way through Junior Year. This is my attempt at teaching myself the various needed topics all comptuer science students should know without having to wait too long to learn what should already be known. Please do not be discouraged and think that this will be an awful project, as I am committed to trying my best at completing it as best I can. 

##Libraries available

### Thread Pool

A full-featured thread pool giving the option to add prioritized tasks, pausing and resuming tasks, and obtaining results from said tasks all with moderate speed and size.

Choose between a static amount of threads or dynamically changing ones (future).

Documentation for version 1.1 available [here](http://theif519.github.io/Thread_Pool_Documentation/).

### String Utils

A basic, non-intrusive and well-tested string manipulations library as well as parsing strings with regular expressions (future).

Documentation for version 1.2 available [here](http://theif519.github.io/String_Utils_Documentation/).

### File Utils

[Unimplemented]

### Networking Utils

[In Development]

Easily create a server with this wonderful abstraction on top of BSD sockets, on however many ports you want, with however many clients you want! Try the NU_Server library!

Maybe you'd rather connect to an existing server as a client instead? Try the NU_Client library!

Or maybe you wish to handle and make HTTP requests easily with an abstraction rather than deal with the lower level stuff? Try NU_HTTP library!

### Data Structures

####Linked List

A generic and general use Linked List utilizing void pointers and callbacks. Sort elements, Iterate through it, and construct and deconstruct them from/to arrays! If you need a dynamic storage of elements that's thread-safe and without a real worry for optimal performance, then this is the best for you.

Documentation for version 1.0 available [here](http://theif519.github.io/Linked_List_Documentation/).

####Priority Blocking Queue

Also known as PBQueue in my library, is a data structure which allows you to enqueue and dequeue elements, in a sorted order, and blocks up to the timeout or if the operation can proceed. Naturally thread-safe.

Documentation for version 1.0 available [here](http://theif519.github.io/Data_Structures_Documentation/Priority_Blocking_Queue/).

####Ring Buffer

[Unimplemented]

####Hash Map

[Unimplemented]

####Deque

[Unimplemented]

### Misc Utils

Beautiful but basic and verbose logging based on log-levels. Use pre-defined prefixes (I.E VERBOSE|INFO|ERROR) or ones of your own!

Documentation for version 1.0 available [here](http://theif519.github.io/Misc_Utils_Documentation/).

## Notes:

Not all packages here are finished, neither do the list of packages here reflect the finished amount.
