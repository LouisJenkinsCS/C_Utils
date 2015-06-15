#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <Thread_Pool.h>
#include <Misc_Utils.h>

int iterations = 0;

MU_Logger_t *logger;

typedef struct {
	unsigned int task_id;
	size_t amount;
} thread_task_t;

static thread_task_t *TT_Create(unsigned int thread_id, size_t amount){
	thread_task_t *task = malloc(sizeof(thread_task_t));
	task->task_id = thread_id;
	task->amount = amount;
	return task;
}

static int *print_hello(thread_task_t *task){
	int *retval = malloc(sizeof(int *));
	*retval = task->task_id;
	int i = 0;
	for(; i<task->amount; i++) {
		iterations++;
		MU_LOG_VERBOSE(logger, "Task %d; Iteration %d;\n", task->task_id, i+1);
		pthread_yield();
	}
	free(task);
	return retval;
}

typedef struct {
	char *message;
	char *priority;
} priority_thread_task_t;

static void *priority_print(void *ptt){
	int i = 0;
	priority_thread_task_t *ptt_p = ptt;
	sleep(1);
	MU_LOG_VERBOSE(logger, "%s Priority Message: %s\n", ptt_p->priority, ptt_p->message);
	sleep(1);
	free(ptt);
	return NULL;
}


static void Test_Non_Timed_Functions(void){
	const unsigned int num_tasks = 25;
	const int runs = 4;
	Result **result = malloc(sizeof(Result *) * num_tasks);
	int i = 0;
	MU_LOG_INFO(logger, "Pausing thread pool to add all tasks!\n");
	Thread_Pool_Pause();
	MU_LOG_INFO(logger, "Thread  Pool Paused to add tasks!\n");
	for(;i<num_tasks ;i++){
		thread_task_t *task = TT_Create(i+1, runs);
		result[i] = Thread_Pool_Add_Task((void *)print_hello, task, TP_NONE);
	}
	Thread_Pool_Resume();
	MU_LOG_INFO(logger, "All tasks added!\nStarted!\n");
	sleep(5);
	MU_LOG_INFO(logger, "Pausing!\n");
	Thread_Pool_Pause();
	sleep(5);
	Thread_Pool_Resume();
	MU_LOG_VERBOSE(logger, "Timed waiting for 3 seconds!\n");
	Thread_Pool_Timed_Pause(3);
	MU_LOG_INFO(logger, "Resumed!\n");
	MU_LOG_INFO(logger, "Injecting a variety of priority tasks!\n");
	for(i = 0; i < num_tasks; i++){ // Prime numbers so only the right one gets triggered.
		priority_thread_task_t *ptt = malloc(sizeof(priority_thread_task_t));
		int priority = TP_NO_PAUSE | TP_NO_RESULT;
		if(i % 2){
			ptt->message = "I have security issues!";
			ptt->priority = "Lowest";
			priority |= TP_LOWEST_PRIORITY;
		} else if(i % 3){
			ptt->message = "I'm a fraud!";
			ptt->priority = "Low";
			priority |= TP_LOW_PRIORITY;
		} else if(i % 5){
			ptt->message = "I'm pretty confident about myself!";
			ptt->priority = "High";
			priority |= TP_HIGH_PRIORITY;
		} else if(i % 7){
			ptt->message = "I'm better than you!";
			ptt->priority = "Highest";
			priority |= TP_HIGHEST_PRIORITY;
		} else{
			ptt->message = "I'm a decent person, overall!";
			ptt->priority = "Medium";
		}
			Thread_Pool_Add_Task(priority_print, ptt, priority);
	}
	sleep(3);
	MU_LOG_VERBOSE(logger, "Timed Pausing for 5 seconds!\n");
	Thread_Pool_Timed_Pause(5);
	MU_LOG_VERBOSE(logger, "Waiting on thread pool for 3 seconds!\n");
	Thread_Pool_Timed_Wait(3);
	Thread_Pool_Resume(); /// Testing whether or not the Thread Pool handles resuming when it's not paused.
	int retval = *((int *)(Thread_Pool_Obtain_Result(result[num_tasks/2 - 1])));
	MU_ASSERT(retval == num_tasks/2, logger);
	Thread_Pool_Wait();
	MU_LOG_INFO(logger, "Total iterations %d; Should be %d; %s\n" ,iterations, num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	for(i=0; i<num_tasks; i++){
		free(result[i]->item);
		Thread_Pool_Result_Destroy(result[i]);
	}
	free(result);
}

int main(void){
	logger = malloc(sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "Thread_Pool_Test_Log.txt", "w", MU_ALL);
	const int num_threads = 4;
	Thread_Pool_Init(num_threads);
	Timer_t *total_time = Timer_Init(1);
	Test_Non_Timed_Functions();
	Timer_Stop(total_time);
	char *total_formatted_time = Timer_To_String(total_time);
	MU_LOG_VERBOSE(logger, "Passed All Tests with total time %s!\n", total_formatted_time);
	Timer_Destroy(total_time);
	free(total_formatted_time);
	Thread_Pool_Destroy();
	return EXIT_SUCCESS;
}