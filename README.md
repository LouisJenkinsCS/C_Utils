# C_Utils
Utilities for C projects

As my mind seems to be as fickle as my luck, C_Utils doesn't really have a set goal of what I want it to be. At first it was a vast library of all possible things I would ever want to use in future projects, but seeing as literally of them actually exist in one way or another, this is going to be my experimentation and person projects in C. They are in essence going to be utilities, but they are not going to be the most efficient. 

C_Utils is going to be a personal side project that I will want to implement as libraries in future side projects. They're not for anyone else to use, just my own.

## String_Utils

### Version

Current version: 1.1

Documentation for the current version can be found [here.](http://theif519.github.io/String_Utils_Documentation/)

### Summary

String Utilities for the C Programming Language, or String_Utils, is my attempt at implementing a more user and newbie-friendly library for string manipulations. 

#### Features

##### Multiple parameter passing.

One of the key features, and where my library differs from all of the others, is that by passing parameters to the function, you can tailor the operation of said function. For example, the option to modify the original string is up to the user. One issue with the standard library, and even others like it, is that they assume that the user wishes to modify the original string. Take for example, the two string literals below.

```
str1 = "Hello ";  
str2 = "World";
strcat(str1, str2)
```
This causes a segmentation fault, as it's read only memory. Take for example my library's concatenation function...

```
str3 = String_Utils_Concat(&str1, str2, NONE)
```
This does not cause a segmentation fault. Note the unary operator here, by passing a reference of the string, it allows the function to change what the string is pointing at, hence being able to modify the original string with the MODIFY parameter.

An example of multiple parameter passing, would be:

```
String_Utils_From_Token("Garbage Text... <parse_this> Random Text <parse_this> More Random Text <PARSE_THIS> Final Random Text ", "<parse_text>", IGNORE_CASE | LAST)
``` 

Which would get the very last token to be parsed, ignoring case for comparison. 

##### Small size library

String_Utils only contains one header file and one source file, well documented and readable.

##### Non intrusive

String_Utils, unlike some other libraries, does not force you to use their own structs that act as a wrapper object for the
string, allowing you to use it with or even without your own wrapper structs. 

##### Memory Management

While this feature is GCC and Clang dependent, by using a defined macro, TEMP, after declaring the name and type of a variable, but before the assignment, as such...

```
char *string TEMP = strdup("Hello World!");
```

Will, when it goes out of scope, for example, of a function, free the string itself. It should be noted that if you change what string the variable is pointing to outside of my library, it will not be able to free it as it the macro works by freeing what is pointed to by the TEMP variable.

## Network_Utils

### Summary

Basic networking. Basically, creating sockets, basic server-client basic structs, send and receiving information over the network. I might also try to attempt serialization, or at the very least send string-representation of objects (might have to work in unison with File and String Utils).

## File_Utils

### Summary

In essence, going to be some basic file reading utilities. I might dabble in parsing files, like XML and JSON, might see if I can make something like JFileChooser where a GUI pops up for you to select your file from (that'd be awesome!). Got a lot on my plate as is though.

## Data_Structures

### Summary

This package will pretty much be my attempts at creating bare-bones, although well-test data structures, such as a double linked list, hash map, and binary heap (binary tree). I plan on creating them so as they are as general and easy to use as humanly possible.

#### Linked_List Features

##### Optional Callbacks for Deleting and Comparing two elements

When created, data structures will not require certain callbacks, however of course it strongly recommended you do pass your own. The default for them will be simple, such having a default deletion that just frees the pointer, while a user-defined callback can more thoroughly ensure properly deletion of each element to prevent memory leaks; a default comparator will just subtract the two item's memory address, to determine if they are equal, with the item declared after being the one, in most cases, that will be considered greater. As is obvious, user-defined callbacks are not needed yet will help immensely for operations like sorting and removing and deleting elements inside of the Linked_List.

##### Multiple parameter passing

For functions that support it, parameters passed to a function will cause the operation to perform different. For instance, if you wish to add an item to the list, by default it is appended to the end of the list. Passing parameters such as FIRST, which prepends it to the beginning of the list, or SORTED | DESCENDING inserts it into the list in a sorted, descending order.

##### Barebones iteration

Features a lightweight iterator built into the Linked_List struct, which allows you to iterate through each and every node in the linked list, appending and prepending, or removing items.

## Memory_Utils

### Summary

This is a theoretical and most likely not going to be finished at all, but as of yet, I've been experimenting with memory management and, in particular, garbage collection. I've already implemented my own simple garbage collector, thanks to a kind blogger who gave out his template, as I'll be trying to improve it as implement it in the other utilities as well.

## Concurrency_Utils

### Summary

A great way to take the opportunity to learn something new, especially in today's day and age, concurrency is king, without a doubt. Single-Threaded applications are a thing of the past, and I've only ever had to work with concurrency once, and while I've learned quite a bit, I don't know nearly enough. Concurrency_Utils be my attempt at implementing a nice and easy abstraction for creating threads, with the goal being that it'll be so easy you don't even have to know what the hell a thread is and be able create and use threads with my library.

Another thing I plan on implementing, which gets me excited, is a thread pool. While others do exist, mine may not be as good, but I plan on making it usable and viable, and of course, easy to use. This way you do not have to deal with threads at all. Just add it to the thread pool's job queue and let it do it's thing. So much stuff to do, so little time.
