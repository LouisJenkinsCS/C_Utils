#define NO_C_UTILS_PREFIX
#include "../stack.h"
#include "../../io/logger.h"
#include "../../threading/thread_pool.h"

#include <unistd.h>
#include <math.h>

static const int num_threads = 4;

/*
	Note, typedef stack_t is taken by POSIX. Hence, we must not use it here.
*/
static struct c_utils_stack *stack = NULL;

static logger_t *logger = NULL;

LOGGER_AUTO_CREATE(logger, "./data_structures/logs/stack_test.log", "w", LOG_LEVEL_ALL);

volatile unsigned long long counter = 0;

volatile unsigned long pops = 0, pushes = 0;

volatile bool running = true;

static void *push_to_stack(void *args) {
	while (running) {
		unsigned long long *i = malloc(sizeof(int));
		assert(i);

		*i = __sync_add_and_fetch(&counter, 1);
		stack_push(stack, i);
		
		pushes++;
	}
	return NULL;
}

static void *pop_from_stack(void *args) {
	while (running) {
		unsigned long long *i = stack_pop(stack);
		
		if (!i) 
			continue;
		
		free(i);
		
		pops++;
	}
	return NULL;
}

int main(void) {	
	stack = stack_create();
	thread_pool_t *tp = thread_pool_create(num_threads);
	int i = 0;
	for (; i < num_threads; i++)
		thread_pool_add(tp, (i % 2 == 0) ? push_to_stack : pop_from_stack, NULL, C_UTILS_NO_RESULT);

	thread_pool_wait(tp, 15);
	
	running = 0;
	
	thread_pool_wait(tp, -1);
	
	thread_pool_destroy(tp);
	
	DEBUG("Max Val: %llu", counter);
	
	stack_destroy(stack, free);
	
	DEBUG("%lu Pops to %lu Pushes, Hence %lu bytes of extra data!", pops, pushes, (pushes - pops) * (sizeof(struct c_utils_node) + sizeof(int)));
	
	return 0;
}