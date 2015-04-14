# C_Utils
Utilities for C projects

C_Utils, in essence, is my attempts at implementing of what I feel is missing, or was overlooked in C. Coming from Java, I've grown so used to having everything done for me, that I was rather astounded and even dishearted at first by how much work you have to do to make something even relatively simple, and how fast things can go wrong with the language not holding your hand. So, this project not only forces me to overcome such problems, it even allows me to solve them in a reusable manor that anyone can use. 

In summar, C_Utils is a personal side project, for future side projects. This is a library that will be, when completed, able to just inject into any project, big or small, and use. It is a one-man project, it can be rather lengthly, but it sure won't stop me from trying.

## *Completed Projects*

### String_Utils

#### Summary

Documentation for String_Utils can be found [here](http://theif519.github.io/String_Utils_Documentation/)

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

Another thing String_Utils offers is a super-cool (IMO, as the creator) idea
of using a mega-struct which serves as a callback-machine, WITH basic documentation.
If contains a callback function to every single function created here, and makes it a lot
easier to call my functions too. For example, lets say you want to concat two strings.
Normally you'd have to call String_Utils_concat(...), which can be rather long if you're
calling it String_Utils_* over and over, even nested in a statement, so a solution
I devised was, lets say you have an instnace of the struct called su, then
it's a lot easier to call su->concat than it is to call String_Utils_concat. 
The next cool part is the documentation! At least in NetBeans, when you 
dereference the struct (or just access the member variable if you prefer that)
you can easily see an alphabetically sorted list of all of the functions as
well a short 1 - 2 sentence description of each function.

I hope you enjoy my first project as much as I will, I worked very hard on it. Hope it shows! Enjoy!

*Future Projects*

## Network_Utils

### Coming soon!

## File_Utils

### Coming soon!

## Data_Structures

### Coming soon!

