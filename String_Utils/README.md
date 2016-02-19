# String_Utils

## Summary

String Utilities is a simple, lightweight and efficient String manipulations library. What this library does to differ from the standard library is support a wider range of string manipulation functions, and differs from it's competitors by allowing some functions to work on non-NULL chracters if the length is specified. 

Everything in this library is completely thread-safe. It also provides some helper-abstractions, like the String typedef, and a simple memory-management solution to temporary strings, the TEMP modifier, which uses a compiler attribute.

Later there will be support for regular expressions.

## General Non-STL dependencies

* GCC/Clang compiler attributes <= TEMP modifier
* MU_Logger <= Logging errors
* MU_Arg_Check <= Checks for bad parameters

### String

* Simple and lightweight String manipulations library
* Thread-Safe
* Verbose logging
