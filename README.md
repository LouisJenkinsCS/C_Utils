# Utilities Package for the C Programming Language

<a href="https://scan.coverity.com/projects/theif519-c_utils">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/8594/badge.svg"/>
</a>

#Summary

##What is C_Utils

A utilities package for the C Programming Language. It features a plethora of tools and abstractions to help ease the development of C projects that is tailored to not only be configurable and versatile, but also easy to use. The utilities package also contains some backports and implementations of features from C++ and from Java. 

##Portability

C_Utils is designed for C99 and with some optionally available with C11. The project however relies on either GCC or Clang, and hence it is not portable to all C99 and C11 compilers, however as those two are the most used of all C compilers, it should be usable from almost any system.

As of now, the library was developed with POSIX and UNIX in mind, however in the future there will be attempts to allow Win32 support as well. Those will be the only two types of platforms (which encompass, once again, a vast majority) I plan to support.

##Abstraction

C_Utils is designed to not be minimal; it is designed to be as configurable and customizable as possible. The library deals and provides many abstractions, with enough configurability to hopefully suit the user's needs.

##Configurability

C_Utils objects can optionally be configured using a configuration object, passed to it's configuration constructor, which normally end with `_conf`. These can be filled out elegantly with C99 initializer lists, and must be zero'd before use. One can toggle the settings of multiple different objects of the same type at runtime, which the preprocessor is generally incapable of doing. This does mean each object has a larger memory imprint, but it's invaluable and small enough to not matter in most circumstances.

##Thread-Safety and Memory-Management (Reference Counting)

C_Utils is currently in the process of a massive conversion to become completely thread safe, and fully integrate it's own reference counting system (which can be used without modifying the data being reference counted). 

C_Utils' thread safety prefers security and generality/versatility of performance and speed, however due to it's configurability, it should allow the user to fine-tune the library to their own performance needs. 

##Changes

As of yet, the massive changes have left the utilities package currently unusable, and with the unfortunate disparity between Stable and Development, and the overall need to make necessary changes, the package will probably not be usable for a long while.

###Why isn't the "working" code in Stable?

Because Stable holds the previous, actually working version of C_Utils, of which I will dub as C_Utils 1.0, and it is more or less archived. Why did I not create a separate branch once this has gotten stable? Well, it never was stable after all of the rapid changes. C_Utils 2.0 will be much more glorious, and it is also better formatted and elegantly designed, also above all else, configurable. Right now the project is more on "display". Once there is a stable version of C_Utils 2.0, I will create a separate branch for it and move it to stable, and C_Utils 1.0 to another branch (probably called "Archive").

#Documentation

The README.md has been significantly shortened, as it will later be used to specify how to install it once it is finished. The documentation can be found [here](http://theif519.github.io/slate/).

It should be noted, that not all things have been implemented yet. The documentation contains ideas and features I plan to implement in the future. Once again, please note, not all of this has been implemented.

#About the Author

Skip if not interested. The author is an ambitious computer science undergraduate who is SUPPOSED to be a senior, but was crazy enough to wait until Junior year to decide a major (and then begin and promptly drop a Math minor by Senior year) who, instead of waiting for god knows how long to learn all of this (if I ever would be taught this in undergraduate school), to teach myself. This project has had over 800 hours invested in it, and doubles as both a tool for learning as well as a valid tool for helping spur along development (once it is finished... if it ever is).

As this package is my personal playground, where I implement, reverse engineer, or "reinvent the wheel" to my heart's content, I may literally never finish this, as there are vastly infinite amount of things to learn. I hope to at least stabalize the utilities package so others may use, but when... well that's another question.