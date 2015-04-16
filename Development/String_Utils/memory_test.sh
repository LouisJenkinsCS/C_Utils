#!/bin/bash

function runtime_failed() {
echo "Program failed to run!"
exit 0
}


clear

echo "Calling valgrind on String_Utils..."

trap runtime_failed EXIT

valgrind --leak-check=full --show-leak-kinds=possible --read-var-info=yes --read-inline-info=yes --log-file=results.txt ./String_Utils

echo "Done!"

