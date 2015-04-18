#include "String_Utils_Garbage_Collector.h"


/* Virtual Machine constructor */
SU_VM *SU_VM_Create(){
	SU_VM *vm = malloc(sizeof(SU_VM));
	vm->stack_size = 0;
	vm->first = NULL; // Initialize first node to null.
	vm->number_of_strings = 0;
	vm->max_to_trigger = AMOUNT_TO_TRIGGER_GC;
	return vm;
}

SU_VM *vm = NULL;

/* Pushes the String object onto the stack. */
void push(SU_VM *vm, String_T *string){
	assert(vm->stack_size < MAX_SIZE);
	vm->stack[vm->stack_size++] = string;
}

/* Pops the String object off the stack. */
String_T *pop(SU_VM *vm){
	assert(vm->stack_size > 0);
	return vm->stack[--vm->stack_size];
}

/* Constructor for the String Object */
String_T *String_Create(SU_VM *vm){
	if(vm->number_of_strings == vm->max_to_trigger) SU_GC(vm);
	String_T *string = malloc(sizeof(String_T));
	string->marked = 0;
	string->next = vm->first;
	vm->first = string; // Since it's a stack, the first is always the newest one.
	vm->number_of_strings++;
	return string;
}

/* Push the passed string on the stack. */
void push_string(SU_VM *vm, char *str){
	/* Initialized relevant fields in String_T */
	String_T *string = String_Create(vm);
	string->string = str;
	/* Push newly initialized String_T on stack */
	push(vm, string);
}

/* Push the array of strings and it's size on the stack. */
/* void push_string_array(SU_VM *vm, const char **array_of_strings, size_t *size){
	// Initialize relevant fields in String_T
	String_T *string = String_Create(vm, STRING_ARRAY);
	string->array_of_strings = array_of_strings;
	string->size = size;
	push(vm, string);
} */

/* Marks all String_T objects in the stack (virtual machine) */
void markAll(SU_VM *vm){
	int i = 0;
	for(i; i < vm->stack_size; i++){
		mark(vm->stack[i]);
	}
}

/* Marks the passed string as still referenced (A.K.A Do not collect) */
void mark(String_T *string){
	if(string->marked) return; // If already marked return.
	string->marked = 1;
}

void sweep(SU_VM *vm){
	String_T **string = &vm->first; // Array of string_t objects.
	while(*string){ // While the current string is not NULL
		if(!(*string)->marked) { // If the current string is not marked...
			String_T *unreached = *string; // No longer contains a reference to it.
			*string = unreached->next; // Sets the current string to the next node.
			free(unreached->string); // Free the object's string.
			free(unreached); // Free this string object.
			--vm->number_of_strings;
		} else {
			// Of course, if it's still marked, it's still in used. So we unmark it for the next 
			// Garbage collection cycle.
			(*string)->marked = 0; // Unmarks current string
			string = &(*string)->next; // Moves array up to the next index.
		}
	}
}

void SU_GC(SU_VM *vm){
	int number_of_strings = vm->number_of_strings;
	markAll(vm);
	sweep(vm);
	vm->max_to_trigger = vm->number_of_strings * 1.5;
	printf("Collected %d objects, %d remaining.\n", number_of_strings - vm->number_of_strings, vm->number_of_strings);
}

void SU_VM_Destroy(SU_VM *vm){
	vm->stack_size = 0;
	SU_GC(vm);
	free(vm);
}