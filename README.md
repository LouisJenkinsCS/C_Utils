# C_Utils
Utilities for C projects

As my mind seems to be as fickle as my luck, C_Utils doesn't really have a set goal of what I want it to be. At first it was a vast library of all possible things I would ever want to use in future projects, but seeing as literally of them actually exist in one way or another, this is going to be my experimentation and person projects in C. They are in essence going to be utilities, but they are not going to be the most efficient. 

C_Utils is going to be a personal side project that I will want to implement as libraries in future side projects. They're not for anyone else to use, just my own.

## *Completed Projects*

### String_Utils

#### Summary

Documentation for String_Utils can be found [here](http://theif519.github.io/String_Utils_Documentation/)

Current version: 1.1

View Documentation for a more up to date summary!

What is String_Utils? String_Utils is basically an attempt at implementing a
very useful, somewhat efficient String library with basic string manipulations
comparators, and utilities offered in object oriented languages. In fact, this
is based on Java's String object's methods, hence the name of most of the functions.

My reason for creating this is, not just for fun, and boy was it ever, but 
also because I haven't found any attempt at creating a string library the way
I did. I rather dislike the way C's String library handles thing, it's too
minimal with how it abstracts and encapsulates it's functions, and plus it 
doesn't even have the basic functions that most people use day-to-day, but they
do however give you the tools to do it yourself, so I decided to. 

String_Utils has a plethora of well-tested (as well as you can with one person
writing this in one week) functions that you've grown to love in OOP languages
like Java, I.E Substring, Index_OF, Split/Join, etc. I attempted to implement
them as closely as they would be in Java, although of course since C and Java
are vastly different languages, with different paradigms, it's impossible
to make it exactly like so. 

Another nice feature is the passing of parameters to tailor the operation the way you want
it if the library function supports it. For instance, one of the problems I had with the standard
C library was that strcat modifies the original string AND returns the pointer to the same
string it modified. I figured, why not just have an option to modify the string passed or not? 
What if I want to work with a string literal? Then with strcat, it would segfault. With my 
concat implementation, you can pass a string literal and not have it attempt to modify it, instead
create a copy of the string for you. In some functions, multiple parameter passing can be passed
with the | operator, I.E 'MODIFY | IGNORE_CASE'

I hope you enjoy my first project as much as I will, I worked very hard on it. Hope it shows! Enjoy!

*Future Projects*

## Network_Utils

### Summary

Basic networking. Basically, creating sockets, basic server-client basic structs, send and receiving information over the network. I might also try to attempt serialization, or at the very least send string-representation of objects (might have to work in unison with File and String Utils).

## File_Utils

### Summary

In essence, going to be some basic file reading utilities. I might dabble in parsing files, like XML and JSON, might see if I can make something like JFileChooser where a GUI pops up for you to select your file from (that'd be awesome!). Got a lot on my plate as is though.

## Data_Structures

### Summary

The basic idea behind this is that, since C is missing any type of dynamic data structure, barring arrays (both examples such as int[] and int **), I took the liberty to do it myself in as much of a reusable way as possible. At the time I am writing this, Data Strctures is still in it's planning phase, however the the current plan goes like. Everything starts with something, and in this project, it begins with a Linked List. An array is too unreliable of a data structure to work with dynamically and generically. Stacks, while an array is a very valid option and easy to implement, can be done in a Linked List. Queues can be done with a Linked List. More complex data structures such as Hash Maps and Binary Trees can also be done with a Linked List. So, the Linked List will be the most important data structure, and will be the most worked on to ensure that it is completely reusable with all implemented data types. 

So far, the Linked List accepts callback functions for special insertions, deletions and comparisons, however to make this feature less of a drag (I.E having to make 3 callback functions just to use a simple Linked List when you may not even need them), default alternatives will also be provided as well in this case. Before expanding on that, it should also be noted that by using a tagged union, the Linked Lists doubles as a single and doubly linked list.

The Linked List's constructor method is planned so that it will take callback functions as arguments, as well as the enumeration depicting whether it's a single or doubly linked list that needs to be created. If any of the callback functions are left as NULL, then default implementations will be used instead, allowing ease of use and also tailoring for specific operations. Linked List also features an iterator which will utilize these callbacks. 

Linked List, insofar is not type safe, so you it is strongly advised that only a single data type be used for each instance of the Linked List.

## Memory_Utils

### Summary

This is a theoretical and most likely not going to be finished at all, but as of yet, I've been experimenting with memory management and, in particular, garbage collection. I've already implemented my own simple garbage collector, thanks to a kind blogger who gave out his template, as I'll be trying to improve it as implement it in the other utilities as well.

## Concurrency_Utils

### Summary

A great way to take the opportunity to learn something new, especially in today's day and age, concurrency is king, without a doubt. Single-Threaded applications are a thing of the past, and I've only ever had to work with concurrency once, and while I've learned quite a bit, I don't know nearly enough. Concurrency_Utils be my attempt at implementing a nice and easy abstraction for creating threads, with the goal being that it'll be so easy you don't even have to know what the hell a thread is and be able create and use threads with my library.

Another thing I plan on implementing, which gets me excited, is a thread pool. While others do exist, mine may not be as good, but I plan on making it usable and viable, and of course, easy to use. This way you do not have to deal with threads at all. Just add it to the thread pool's job queue and let it do it's thing. So much stuff to do, so little time.
