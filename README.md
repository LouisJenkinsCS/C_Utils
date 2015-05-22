# C_Utils
Utilities for C projects

As my mind seems to be as fickle as my luck, C_Utils doesn't really have a set goal of what I want it to be. At first it was a vast library of all possible things I would ever want to use in future projects, but seeing as literally of them actually exist in one way or another, this is going to be my experimentation and person projects in C. They are in essence going to be utilities, but they are not going to be the most efficient. 

C_Utils is going to be a personal side project that I will want to implement as libraries in future side projects. They're not for anyone else to use, just my own.


## Network_Utils

### Summary

Basic networking. Basically, creating sockets, basic server-client basic structs, send and receiving information over the network. I might also try to attempt serialization, or at the very least send string-representation of objects (might have to work in unison with File and String Utils).

## File_Utils

### Summary

File_Utils in essence is going to be a robust, semi-lightweight library for manipulating files. It will feature things that people will want to use in a language like C that can easily be found in other Object-Oriented languages.

#### Features

##### File_Info Structure

This structure wraps the FILE * in an object-like structure, which will also contain an array of buffers that holds a string up to the first newline it encounters. This way, it will make operations like reading the next line possible and easy to do. Also, do to this, it also makes deletion of lines possible, and appending of data even easier. File_Info keeps track of the current line and current character in the line so you don't have to.

##### Modification of any line in a document.

With File_Info, it will allow you to delete, modify and append new lines anywhere in the document, even append characters and strings to the current line as well. It will even allow you to replace a line entirely with one of your own.

##### View a document before writing it.

It is a rather inefficient way, but you can even receive a massive string of the document with newlines intact. This allows you to print it yourself, or any other tool to view the text beforehand.

##### Parameter Passing

Tailor your File how you want it. With certain parameters, you can append text from a current position, from the beggining or at the end of the line without having to move current character forward to the end yourself. You can even receive a copy of the document up to your current line, between two lines, or even before.

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
