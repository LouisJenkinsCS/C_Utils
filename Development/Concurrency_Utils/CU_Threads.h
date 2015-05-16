#ifndef CU_THREADS_H
#define CU_THREADS_H

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct Thread Thread;

struct Thread {
	pthread_t thread;
	pthread_mutex_t mutex;

};

Thread *Thread_Create(void *(*callback)(void *args), void *args, int attributes);

int Thread_Destroy(Thread *this);
#endif /* End CU_THREADS_H */