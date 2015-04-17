#!/bin/bash
echo "Compiling String_Utils"
gcc -g String_Utils.c String_Utils_Tests.c -o String_Utils
echo "Done!"
