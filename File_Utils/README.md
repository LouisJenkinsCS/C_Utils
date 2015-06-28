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
