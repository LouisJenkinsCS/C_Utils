#ifndef LINKED_LIST_H
#define LINKED_LIST_H


/* Typedef for the structs */
typedef struct Node Node;
typedef struct Linked_List Linked_List;
typedef struct Iterator Iterator;
typedef enum Node_T Node_T;

/* Typedef for Linked_List function pointers */
typedef int (*Linked_List_Add_Callback)(Linked_List *this, void *item);
typedef int (*Linked_List_Delete_Callback)(Linked_List *this, Node *node);
typedef int (*Linked_List_Compare_Callback)(Linked_List *this, void *item_one, void *item_two);


/* Typedef for Iterator function pointers */
/// Gets the next item... unfortunately, can't forward the node to the next.
/// #define Iterator_Get_Current(this, type)((type *)((this)->(current)->item))

typedef void *(*Iterator_get_next)(Iterator *iterator));
typedef void *(*Iterator_get_previous)(Iterator *iterator));
typedef void *(*Iterator_get_last)(Iterator *iterator));
typedef void *(*Iterator_get_first)(Iterator *iterator));

typedef int (*Iterator_delete_current)(Iterator *iterator);
typedef int (*Iterator_append_current)(Iterator *iterator, void *item);
typedef int (*Iterator_prepend_current)(Iterator *iterator, void *item);

/* The struct that holds the next and previous pointers. The type of node,
   A.K.A Node_T, will determine which struct in the union will be used. */
struct Node {
	void *item;
	union {
		/// For SINGLE Node_T
		struct {
			Node *single_next;
		};
		/// For DOUBLE Node_T
		struct{
			Node *double_next;
			Node *double_prev;
		};
	};
};

/* Keep track of the only two possible types. */
enum Node_T {
	/// Obviously, for singly linked lists.
	SINGLE,
	/// For doubly linked lists.
	DOUBLE,
	/// For Tree-like data structures (I.E Binary Tree)
	TREE
}

/* The Linked List structure which holds the basic information, the first and last node, and
   some callback functions which are to be used to enforce polymorphism. */
struct Linked_List{
	/// The very first node.
	Node *first;
	/// The very last node.
	Node *last;
	/// The current size of the linked list.
	size_t size;
	/// Type of node
	Node_T node_type;
	/// Add callback.
	Linked_List_Add_Callback add;
	/// Delete callback.
	Linked_List_Delete_Callback del;
	/// Comparator callback.
	Linked_List_Compare_Callback cmp;
};

/* A barebones iterator */
struct Iterator {
	/// The linked list to iterate through.
	Linked_List *list;
	/// The current node in the list the iterator is on.
	Node *current;
	/// Callback to get next
	Iterator_get_next next;
	/// Callback to get previous (only if doubly linked list!)
	Iterator_get_previous prev;
	/// Callback to get the last node.
	Iterator_get_last last last;
	/// Callback to get first node.
	Iterator_get_first first first;
	/// Callback to delete the current node.
	Iterator_delete_current del;
	/// Callback to append an item at the current index.
	Iterator_append_current append;
	/// Callback to prepend an item at the current index.
	Iterator_prepend_current prepend;
};

/* Create the Linked List with the the supplied callbacks. If NULL is passed for any but Node_T, the default implementation 
   for the callback will be used in it's place. */
Linked_List *Linked_List_create(Linked_List_Add_Callback add, Linked_List_Delete_Callback del, Linked_List_Compare_Callback cmp, Node_T node_type);

/* The standard, default callback used in place of a null Add_Callback parameter. */
int Linked_List_default_add(Linked_List *this, void *item);

/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_default_delete(Linked_List *this, Node *node);

/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_default_compare(Linked_List *this, void *item_one, void *item_two);

/* Returns an iterator for the linked list. This function will also set the required callbacks as well. */
Iterator *Linked_List_get_iterator(Linked_List *this);

/* Returns the next object in the iterator. */
void * Iterator_get_next(Iterator *iterator);

/* Returns the previous entry in the iterator if and only if it is a double linked list. */
void * Iterator_get_previous(Iterator *iterator);

/* A no-operation function that gets set on the previous callback if it is a single linked list. Returns NULL. */
void * Iterator_nop(Iterator *iterator);

/* Moves the current node to be the last, if it isn't already. */
void * Iterator_get_last(Iterator *iterator);

/* Moves the current node to be the first, if it isn't already. */
void * Iterator_get_first(Iterator *iterator);

/* Deletes the current node from the list. */
int Iterator_delete_current(Iterator *iterator);

/* Appends the item to the current node (as a node) */
int Iterator_append_current(Iterator *iterator, void *item);

/* Prepends the item to the current node (as a node) */
int Iterator_prepend_current(Iterator *iterator, void *item);

/* Deletes this iterator. Does not destroy the linked list along with it. */
void Iterator_destroy(Iterator *iterator);

/* Destroys the linked list along with all of it's contents. Make sure you get everything from the linked list before
   calling this! */
void Linked_List_destroy(Linked_List *list);










#endif /* LINKED_LIST_H */