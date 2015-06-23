## Data_Structures

### Summary

This package will pretty much be my attempts at creating bare-bones, although well-test data structures, such as a double linked list, hash map, and binary heap (binary tree). I plan on creating them so as they are as general and easy to use as humanly possible.

#### Linked_List Features

##### Optional Callbacks for Deleting and Comparing two elements

When created, data structures will not require certain callbacks, however of course it strongly recommended you do pass your own. The default for them will be simple, such having a default deletion that just frees the pointer, while a user-defined callback can more thoroughly ensure properly deletion of each element to prevent memory leaks; a default comparator will just subtract the two item's memory address, to determine if they are equal, with the item declared after being the one, in most cases, that will be considered greater. As is obvious, user-defined callbacks are not needed yet will help immensely for operations like sorting and removing and deleting elements inside of the Linked_List.

##### Multiple parameter passing

For functions that support it, parameters passed to a function will cause the operation to perform different. For instance, if you wish to add an item to the list, by default it is appended to the end of the list. Passing parameters such as FIRST, which prepends it to the beginning of the list, or SORTED | DESCENDING inserts it into the list in a sorted, descending order.

##### Barebones iteration

Features a lightweight iterator built into the Linked_List struct, which allows you to iterate through each and every node in the linked list, appending and prepending, or removing items.
