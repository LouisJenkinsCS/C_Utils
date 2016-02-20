#include <MU_Logger.h>
#include <MU_Scoped_Lock.h>
#include <pthread.h>

static MU_Logger_t *logger;

// Intentionally not atomic counter
static volatile unsigned long long counter = 0;

static unsigned long long iterations = 10000000;
static unsigned int num_threads = 4;

static void *thread_func(void *lock){
	MU_Scoped_Lock_t *s_lock = lock;
	for(int i = 0; i < iterations; i++){
		MU_SCOPED_LOCK(s_lock) counter++;
	}
	return NULL;
}

MU_LOGGER_AUTO_CREATE(logger, "./Misc_Utils/Logs/MU_Scoped_Lock_Test.log", "w", MU_ALL);

int main(void){
	pthread_spinlock_t lock;
	pthread_spin_init(&lock, 0);
	MU_Scoped_Lock_t *s_lock = MU_SCOPED_LOCK_FROM(&lock);
	pthread_t threads[num_threads];
	for(int i = 0; i < num_threads; i++){
		pthread_create(threads + i, NULL, thread_func, s_lock);
	}
	for(int i = 0; i < num_threads; i++){
		pthread_join(threads[i], NULL);
	}
	MU_LOG_INFO(logger, "The counter, which should be %llu is %llu", iterations * num_threads, counter);
	free(s_lock);
	return 0;
}