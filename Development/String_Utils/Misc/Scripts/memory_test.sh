#!/bin/bash

clear

echo "Calling valgrind on String_Utils..."

valgrind --leak-check=full --show-leak-kinds=possible --read-var-info=yes --read-inline-info=yes --log-file=results.txt ./String_Utils

echo "Done!"

