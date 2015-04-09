# C_Utils
Utilities for C projects

This is a personal project I am undertaking to further my own knowledge and understanding of programming. What better way to do that then to implement everything from scratch? Although, really, a lot of it isn't really "scratch" per se because quite a bit of it calls on already existing code and library functions. So, really, my C_Utils will not necessarily be something original or groundshattering, it instead will make it easier for the user (A.K.A Me) to use already existing functions without having to worry about the nitty gritty details. I suppose you could say I'm making it easier by doing all of the harder details behind the scenes (for myself).

## *Current projects*

### String_Utils

#### Summary

String_Utils is nothing truly special on its own, but what it does boast is the amount of effort to be saved by using it. While everything in this library is easily accessible without it, it does save quite a bit of time in the long run, as everything, as long as the correct parameters are passed, are going to be done for you. For instance, lets say you wish to tokenize a string into an array of string literals? Well, most likely you already know how, but instead of having to go through the trouble, it's split() function returns an already allocated array of string literals for you. Hence, you spend less time on the tedium and more on actual coding! At least, I know I will.

String_Utils will also be based on Java's String object, which is why the functions are named as they are. To simulate calling a method, I added a ton of callback functions for these functions, making them a lot easier to be called, on top of that it supports parameter passing which helps you taylor how exactly you want your string returned/modified. For example

```
String_Utils s = String_Utils_Create(); // The 'constructor' is kind of the longest function you ever have to call.
char *str = "Hello ";
char *new_string = s->concat(str, " World", NO_MODIFY); // Returns a new string, str is not modified.
printf("new_string value: %s; str value: %s", new_string, str); // For proof
s->concat(str, " World", MODIFY); // str is modified
printf("str value: %s", str); // also for proof
```

For more proof, please check the assertion for each String_Utils when they are released!

## Network_Utils

### Coming soon!

