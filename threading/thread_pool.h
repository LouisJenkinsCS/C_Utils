#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/*
 *  A simple, yet powerful Thread Pool implementation.
 * 
 * This thread pool differs from other's in the fact that not only does it use a 
 * priority queue for tasks, it also allows you to the obtain the result from an operation
 * if desired and wait on them through my own implementations of Win32 Events.
 * 
 * It also uses bit flag passing with the bit-wise OR operator (|), to fine-tune
 * the usage of the thread pool.
 * 
 * For example, if you wanted to add a task of above average (high) priority, but you
 * do want the result, you simply pass the flag TU_HIGH_PRIORITY. However, lets say you 
 * do not want the result but still want it to be high priority, then it's as simple
 * as TU_HIGH_PRIORITY | TU_NO_RESULT. 
 * 
 * Now, lets say you don't want to deal with prioritized tasks and want the result, which
 * is the default, you can simply pass TP_NONE or 0 for flags.
 * 
 * Finally it has the ability to pause and resume the thread pool, or even pause
 * the thread pool for a specified amount of time. Trivial, yet useful in certain situations.
 * You can even wait on the thread pool, with the option of a timeout, allowing you to
 * finetune whether you want to block indefinitely or poll.
 * 
 * The default behavior for the thread pool is the following...
 * - A c_utils_result object is returned
 * - The task is added with a normal/medium priority
 * - You MUST free the c_utils_result object when finished.
 */

#include <stdbool.h>

typedef void *(*c_utils_task)(void *);

struct c_utils_result;

struct c_utils_thread_pool;

#ifdef NO_C_UTILS_PREFIX
typedef struct c_utils_thread_pool thread_pool_t;
typedef struct c_utils_result result_t;
#define thread_pool_create(...) c_utils_thread_pool_create(__VA_ARGS__)
#define thread_pool_add(...) c_utils_thread_pool_add(__VA_ARGS__)
#define thread_pool_clear(...) c_utils_thread_pool_clear(__VA_ARGS__)
#define thread_pool_pause(...) c_utils_thread_pool_pause(__VA_ARGS__)
#define thread_pool_resume(...) c_utils_thread_pool_resume(__VA_ARGS__)
#define thread_pool_wait(...) c_utils_thread_pool_wait(__VA_ARGS__)
#define thread_pool_destroy(...) c_utils_thread_pool_destroy(__VA_ARGS__)
#define result_get(...) c_utils_result_get(__VA_ARGS__)
#define result_destroy(...) c_utils_result_destroy(__VA_ARGS__)
#endif

/// Will not return a Result upon submitting a task.
const int NO_RESULT = 1 << 1;
/// Flags the task as lowest priority.
const int LOWEST_PRIORITY = 1 << 2;
/// Flags the task as low priority.
const int LOW_PRIORITY = 1 << 3;
/// Flags the task as high priority.
const int HIGH_PRIORITY 1 << 4;
/// Flags the task as highest priority.
const int HIGHEST_PRIORITY = 1 << 5;

/**
 * Returns a newly allocated thread pool with pool_size worker threads.
 * @param pool_size Number of worker threads, minimum of 1.
 * @return The thread pool instance, or NULL on allocation error or initialization error.
 */
struct c_utils_thread_pool *c_utils_thread_pool_create(size_t pool_size);

/**
 * Adds a task to the thread pool. While it requires a void *(*callback)(void *args)
 * function pointer, casting a function to void *, like (void *)function, will work
 * but has undefined behavior.
 * @param task Callback function to be called as the task.
 * @param args Arguments to pass to the thread pool.
 * @param flags TP_NONE | TP_LOWEST_PRIORITY | TP_LOW_PRIORITY | TP_HIGH_PRIORITY
 * | TP_HIGHEST_PRIORITY | TP_NO_RESULT.
 * @return The result from the task to be obtained later or NULL if TP_NO_RESULT.
 */
struct c_utils_result *c_utils_thread_pool_add(struct c_utils_thread_pool *tp, c_utils_task task, void *args, int flags);

/**
 * Destroys the Result from a task.
 * @param result Result to be destroyed.
 * @return true on success, false if not.
 */
bool c_utils_result_destroy(struct c_utils_result *result);

/**
* Clears the thread pool of all tasks.
* @param tp Thread Pool instance.
* @return true if successful, false if not.
*/
bool c_utils_thread_pool_clear(struct c_utils_thread_pool *tp);

/**
 * Destroys the current thread pool, waiting for the current tasks to be finished.
 * @return 1 on success.
 */
bool c_utils_thread_pool_destroy(struct c_utils_thread_pool *tp);

/**
 * Obtain the result from the task, blocking until it is ready or until the time 
 * elapses, in which it will return NULL.
 * @param result Result to obtain result from.
 * @param timeout Maximum amount of time to block for if result isn't ready.
 * @return The item stored inside the result, or NULL if timeout or thread pool is shutdown.
 */
void *c_utils_result_get(struct c_utils_result *result, long long int timeout);

/**
 * Block until all tasks are finished or timeout ellapses.
 * @param tp Thread pool instance.
 * @param timeout The max amount of time to block for, infinite if -1.
 * @return true if the thread pool is finished, false if not.
 */
bool c_utils_thread_pool_wait(struct c_utils_thread_pool *tp, long long int timeout);

/**
 * Pause the thread pool, and prevent worker threads from working. It does not stop threads from executing
 * their current tasks.
 * @param tp Thread pool instance.
 * @param timeout The max amount of time to block for, infinite if -1.
 * @return true if paused, false if not.
 */
bool c_utils_thread_pool_pause(struct c_utils_thread_pool *tp, long long int timeout);

/**
 * Resume all worker threads that are paused.
 * @param tp Thread pool instance.
 * @return true on success, false if not.
 */
bool c_utils_thread_pool_resume(struct c_utils_thread_pool *tp);

#endif /* END THREAD_POOL_H */