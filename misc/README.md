#Misc_Utils

##Version

Current Version: 1.0

Documentation available [here](http://theif519.github.io/Misc_Utils_Documentation/).

## Summary

Miscallaneous Utilities for the C Programming Language, or Misc_Utils, is a collection, albeit a small one, of utilities that aren't big enough to deserve it's own package.

Inside, it contains basic logging macros to log information to a file, a debugging macro to output information to stderr in a prettified format. It should also be noted that the logging functions also show the name of the file and the line where the macro is invoked, thanks to GCC magic.

Inside also is a timer, which is extremely bare bones for now, and allows you to start, stop and obtain the total time in string format.

### Features

####Make debugging easier!

Misc_Utils allows you to log information to disk, as well as the line number and name of the file. It also allows you to time your programs as well. Misc_Utils also features a very simple debug macro for if you don't want to log to a file and just want to log to stderr, in fact, you can even log everything to stdout or stderr as well.
