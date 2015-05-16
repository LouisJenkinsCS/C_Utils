#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct Thread_Pool Thread_Pool;

typedef struct Result Result;

typedef struct Sub_Process Sub_Process;

typedef void *(*thread_callback)(void *args);

struct Thread_Pool {
	/// Array of threads.
	pthread_t *threads;
	/// Amount of threads currently created, A.K.A Max amount.
	size_t thread_count;
	/// Amount of threads currently active.
	size_t active_threads;
};

struct Result {
	/// Determines whether or not it has been processed.
	unsigned char ready;
	/// The return type, NULL until ready.
	void *item;
};

struct Sub_Process {
	/// Result to be processed.
	Result *result;
	/// Thread callback to be processed
	thread_callback cb;
	/// Arguments to be passed to the callback
	void *args;
};

static void *Process_Result(void *args){
	Sub_Process *proc = args;
	// Nested thread, find a better way to do this.
	pthread_t temp_thread;
	// Second thread.
	pthread_create(&temp_thread, NULL, proc->cb, proc->args);
	// Obtain result from second thread.
	pthread_join(&temp_thread, proc->result->item);
	// Reasoning for returning NULL? The result has already been returned to the user.
	// Just flags as ready.
	proc->result->ready = 1;
	return NULL;
}

Result *TP_Add_Task(thread_pool *tp, thread_callback cb, void *args, int parameters){
	Result *result = malloc(sizeof(Result));
	result->ready = 0;
	result->item = NULL;
	// Thread attributes based on parameters passed.
	pthread_attr_t attribute = NULL;
	Sub_Process *proc = malloc(sizeof(Sub_Process));
	proc->result = result;
	proc->cb = cb;
	proc->args = args;
	// First thread.
	pthread_create(tp->threads[(tp->active_threads)++], attribute, )
}

void *test_function(void *args){
	printf("%d\n", *(int *)args);
	return NULL;
}

int main(void){
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	// Create an array of arguments to pass.
	void **array_of_args = malloc(sizeof(void *) * 2);
	int i = 0;
	for(; i < 100; i++){
		pthread_create(&thread, NULL, test_function, (void *) &i);
	}
	return 0;
}