# String_Utils

## Version

Current version: 1.2

Documentation for the current version can be found [here.](http://theif519.github.io/String_Utils_Documentation/)

## Summary

String_Utils is a string library based off of Java's own String library, except the library works on char * over a string object (or struct). It is a lightweight, relatively speedy library which relies upon the libc and glibc.

String_Utils also features passing flags with the '|' operator, for example, SU_IGNORE_CASE|SU_LAST does exactly as it says, ignore case for string comparison, and return the last string that matches the criteria. For default behavior, you pass SU_NONE flag instead.

Some functions require passing the address of the string (&) for possible modification of the original string, as String_Utils never modifies the string passed, by default, so it is safe to use string literals unless passing SU_MODIFY flag.

### Features

#### Multiple parameter passing.

One of the key features, and where my library differs from all of the others, is that by passing parameters to the function, you can tailor the operation of said function. For example, the option to modify the original string is up to the user. One issue with the standard library, and even others like it, is that they assume that the user wishes to modify the original string. Take for example, the two string literals below.

```
str1 = "Hello ";  
str2 = "World";
strcat(str1, str2);
```
This causes a segmentation fault, as it's read only memory. Take for example my library's concatenation function...

```
str3 = String_Utils_Concat(&str1, str2, SU_NONE);
```
This does not cause a segmentation fault. Note the unary operator here, by passing a reference of the string, it allows the function to change what the string is pointing at, hence being able to modify the original string with the MODIFY parameter.

An example of multiple parameter passing, would be:

```
String_Utils_From_Token("Garbage Text... <parse_this> Random Text <parse_this> More Random Text <PARSE_THIS> Final Random Text ", "<parse_text>", SU_IGNORE_CASE | SU_LAST);
``` 

Which would get the very last token to be parsed, ignoring case for comparison. 

#### Small size library

String_Utils only contains one header file and one source file, well documented and readable.

#### Non intrusive

String_Utils, unlike some other libraries, does not force you to use their own structs that act as a wrapper object for the
string, allowing you to use it with or even without your own wrapper structs. 

#### Memory Management

While this feature is GCC and Clang dependent, by using a defined macro, TEMP, after declaring the name and type of a variable, but before the assignment, as such...

```
char *string TEMP = strdup("Hello World!");
```

Will, when it goes out of scope, for example, of a function, free the string itself. It should be noted that if you change what string the variable is pointing to outside of my library, it will not be able to free it as it the macro works by freeing what is pointed to by the TEMP variable.
