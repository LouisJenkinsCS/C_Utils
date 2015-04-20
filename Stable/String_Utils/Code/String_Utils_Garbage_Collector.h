#ifndef SU_GC_H
#define SU_GC_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

/*
 * @Author: Louis Jenkins
 * @Version: 1.0
 * @Brief: String_Utils Garbage Collector
 * 
 * String_Utils_Garbage_Collector, which I was denote as SU_GC, is an experiment of mine
 * where it will attempt to make strings allocated from String_Utils functions as self-managed
 * as possible. Unfortunately, it's not fully implemented, and might not be for a while, but
 * I do have my sights on a goal. Currently implemented features will be listed below...
 * 
 * 1) Very simple and basic virtual machine that, so far, has a stack for any allocated
 *    temporary variables pushed on it.
 *  1A) Uses a Mark-And-Sweep algorithm to determine what should be freed.
 * 
 * Eventually going to be implemented features:
 * 
 * 2) A heap to allocate more permanent long-term strings on it.
 *  2A) Based on a binary heap (tree) with key/value pairing for each to find and traverse nodes.
 *  2B) Will be allocated on this heap with a custom MALLOC (SU_MALLOC).
 *  2C) Will keep track of everything allocated, eliminating memory leaks.
 * 3) Multi-threaded SU_GC and thread-safety
 *  3A) SU_GC will run on it's own thread, waiting until a certain condition is met to algorithmically mark-and-sweep.
 *  3B) Will be a thread-safe operation, with each thread being able to either share a virtual machine, or get it's own.
 * 
 * Benefits of SU_GC (Unproven)
 * 
 * 1) Easier to manage strings allocated, leaving behind the possibility of memory leaks.
 *  1A) Temporary strings allocated in functions are pushed on a stack, then popped off after. The
 *      benefit of this being that SU_GC does not free them immediately, saving resources
 *      and time by doing so at a later date (In it's own thread [Not implemented])
 *  1B) Strings allocated on the heap have a 0% chance of being leaked when the application exits, even
 *      if the string's reference is lost without being freed. So, every string will be freed when the
 *      virtual machine shuts down, regardless of where in your program you initialized it.
 * 2) High performance with no possibility of hang ups.
 *  2A) Each instance of the SU_GC will run on it's own thread, separate from current thread
 *      in use [Not Implemented].
 *  2B) SU_GC waits until a condition variable is set to collect objects on the stack and heap.
 * 3) Educational!
 *  3A) This is purely for me, the creator, I never thought I'd ever implement something like SU_GC,
 *      but it is, needless to say, a very educational experience.
 */

/**
 * String_Pointer, aliased as String_T is the structure which acts as both a container
 * for the string (char *) inside of it, but also as a linked list which points to the next
 * String_T object. This way it maintains a reference to the object even after it gets removed
 * from the Virtual Machine's stack. 
 * 
 * Marked is the bit that determines whether or not a reference is held on this object, hence if it
 * is unmarked, it gets collected, if it is marked, then it isn't (as it is currently in use). In the future, 
 * there will be an attempt to implement a way to determine whether an object is in use in a more
 * automatic way, but as is, all of this is abstracted through String_Utils functions.
 */
typedef struct String_Pointer {
    /// Node that points to the next object.
    struct String_Pointer *next;
    /// Marked bit
    unsigned char marked;
    /// The string to be stored when allocated.
    char *string;
} String_T;

/**
 * MAX_SIZE: The maximum amount of items on the stack at once or else triggers a stack overflow assertion.
 */
#define MAX_SIZE 1024

/**
 * The initial amount to trigger the GC. After each trigger, the amount will
 * increase by 1.5x. So first GC is 4, then 6, then 9, etc.
 */
#define AMOUNT_TO_TRIGGER_GC 4

/**
 * The structure that is the virtual machine. So far it contains the pointer
 * to the first (A.K.A the top of the stack) which acts as a linked list to collect
 * strings released from the stack. The stack is for pushing and popping off temporary variables.
 * Also holds the current number of strings on the stack, the max amount of allocations
 * to trigger, and size of the current stack.
 */
typedef struct {
    /// The pointer to the very first node in the stack.
    String_T *first;
    /// The stack of all allocated String_T
    String_T *stack[MAX_SIZE];
    /// Current number of strings allocated on the stack.
    int number_of_strings;
    /// The number needed to trigger the Garbage Collector.
    int max_to_trigger;
    /// The current size of the stack.
    int stack_size;
} SU_VM;

/**
 * Extremely experimental purposes. This is a global variable that was used for the testing phase.
 */
extern SU_VM *vm;

/**
 * The constructor for the virtual machine.
 * @return The pointer to the constructed virtual machine.
 */
SU_VM *SU_VM_Create();

/**
 * Push a String_T object on the stack. Do not use directly!
 * @param vm Virtual machine
 * @param string String_T object to push.
 */
void push(SU_VM *vm, String_T *string);

/**
 * Pops the last String_T pushed off of the stack.
 * @param vm Virtual machine
 * @return The last object on the stack.
 */
String_T *pop(SU_VM *vm);

/**
 * Pop a certain amount (size) objects off of the stack.
 * @param vm Virtual machine.
 * @param size Amount of String_T objects to pop off the stack.
 * @return An array of String_T objects.
 */
String_T **pop_n(SU_VM *vm, size_t size);

/**
 * A void return type that pops multiple String_T objects off of the stack without
 * having to worry about the overhead of an array of String_T objects.
 * @param vm Virtual machine
 * @param size Amount of String_T objects to push off stack.
 */
void pop_vn(SU_VM *vm, size_t size);

/**
 * Create a String_T object. Do not use directly!
 * @param vm Virtual machine.
 * @return An allocated String_T object.
 */
String_T *String_Create(SU_VM *vm);

/**
 * Push the given string on the stack.
 * @param vm Virtual machine.
 * @param str The string to be pushed.
 */
void push_string(SU_VM *vm, char *str);

/**
 * Variadic function to push more than one string on the stack.
 * @param vm Virtual machine
 * @param size Amount of strings you are pushing
 * @param str First string to push.
 * @param ... Rest of strings
 */
void push_strings(SU_VM *vm, size_t size, char *str, ...);

/**
 * Marks all String_T objects on the stack. Do Not Use!
 * @param vm Virtual machine
 */
void markAll(SU_VM *vm);

/**
 * Mark the given String_T object; Do Not Use!
 * @param string String to mark.
 */
void mark(String_T *string);

/**
 * Collects all objects unmarked; also unmarks every String_T object. Do Not Use!
 * @param vm
 */
void sweep(SU_VM *vm);

/**
 * Trigger garbage collection. Calls markAll and Sweep.
 * @param vm Virtual machine.
 */
void SU_GC(SU_VM *vm);

/**
 * Deconstructor for the virtual machine. Will wipe everything in it's stack first.
 * @param vm Virtual Machine
 */
void SU_VM_Destroy(SU_VM *vm);
#endif /* SU_GC_H */