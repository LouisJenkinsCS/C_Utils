#define C_UTILS_NO_PREFIX

#include "../heap.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static const int max_ints = 1000;

static int compare_ints(void *f, void *s) {
	return *(int *)f - *(int *)s;
}

int main(void) {
	heap_t *heap = heap_create(compare_ints);
	int arr[max_ints] = {0};

	srand(time(NULL));

	for(int i = 0; i < max_ints; i++) {
		arr[i] = rand() % 100 + 1;
		heap_insert(arr + i);
	}

	int last = 0;
	for(int i = 0; i < max_ints; i++) {
		int curr = heap_remove(heap);
		if(last)
			assert(curr >= last);

		last = curr;
	}

	heap_destroy(heap);
}