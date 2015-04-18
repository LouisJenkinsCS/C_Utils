#ifndef SU_GC_H
#define SU_GC_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
typedef struct String_Pointer {
	struct String_Pointer *next; // Node that points to the next object.
	unsigned char marked; // Marked bit
	char *string; // The string to be stored when allocated.
} String_T;

#define MAX_SIZE 1024
#define AMOUNT_TO_TRIGGER_GC 4

/* Struct to hold the objects allocated on the stack. */
typedef struct {
	String_T *first; // The pointer to the very first node in the stack.
	String_T *stack[MAX_SIZE]; // The stack of all allocated String_T
	int number_of_strings; // Current number of strings allocated on the stack.
	int max_to_trigger; // The number needed to trigger the Garbage Collector.
	int stack_size;
} SU_VM; // String_Utils_Virtual_Machine

extern SU_VM *vm;
/* Virtual Machine constructor */
SU_VM *SU_VM_Create();

/* Pushes the String object onto the stack. */
void push(SU_VM *vm, String_T *string);

/* Pops the String object off the stack. */
String_T *pop(SU_VM *vm);

/* Constructor for the String Object */
String_T *String_Create(SU_VM *vm);

/* Push the passed string on the stack. */
void push_string(SU_VM *vm, char *str);

/* Marks all String_T objects in the stack (virtual machine) */
void markAll(SU_VM *vm);

/* Marks the passed string as still referenced (A.K.A Do not collect) */
void mark(String_T *string);

/* Goes through the linked list of allocated String_T's, and will free what is not marked. */
void sweep(SU_VM *vm);

/* Garbage Collector for String_Utils */
void SU_GC(SU_VM *vm);

void SU_VM_Destroy(SU_VM *vm);
#endif /* SU_GC_H */