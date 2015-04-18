#include "String_Utils_Garbage_Collector.h"
#include <string.h>
#include <stdio.h>
char *Test_Copy(char *string){
	if(vm == NULL) vm = SU_VM_Create();
	char *temp = malloc(strlen(string) + 1);
	strcpy(temp, string);
	push_string(vm, temp);
	return temp;
}

/* This function is one valid application, even if there is a better function to read from a file
   than fgets, since fgets returns an EOF, and sets buffer to null, normally it'd end up being lost.
   However, since the virtual machine has the original string pushed on it, it is efficiently and
   effectively capable of removing the, albeit very small, memory leak. Yay!

   Note: The memory leak is in proportion to the size of the buffer. To exaggerate this, I have a 
   1MB buffer, which fills up multiple times. At the end of this EOF, it would leave a 1MB memory leak.
   Now apply that to gigabytes of text... and most likely you'd use a specialized library. Still, carrying on. */
void test_GC_Buffered_Input(void){
	vm = SU_VM_Create();
	FILE *fd;
	fd = fopen("/home/Moltar/Long_Text.txt", "r");
	if(fd == NULL) exit(EXIT_FAILURE);
	char *temp = malloc(1000000);
	push_string(vm, temp); // In the case of fgets receiving EOF
	int amount = 0;
	int i = 0;
	while(fgets(temp, 1000000, fd)) { // fgets sets temp to NULL on EOF
		//push_string(vm, temp);
		//printf("Received input #%d\n", ++amount);
		//pop(vm);
	}
	fclose(fd);
	pop(vm); // Pop from stack.
	printf("Received %d inputs, lost references to all of them!\n", amount);
	SU_VM_Destroy(vm);
}

int main(void){
	test_GC_Buffered_Input();
	
	return 0;
}