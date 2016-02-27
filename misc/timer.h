#ifndef C_UTILS_TIMER_H
#define C_UTILS_TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct c_utils_timer;

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_timer timer_t;

/*
	Functions
*/
#define timer_init(...) c_utils_timer_init(__VA_ARGS__)
#define timer_start(...) c_utils_timer_start(__VA_ARGS__)
#define timer_stop(...) c_utils_timer_stop(__VA_ARGS__)
#define timer_string(...) c_utils_timer_string(__VA_ARGS__)
#define timer_destroy(...) c_utils_timer_destroy(__VA_ARGS__)

#endif

/**
 * Creates a new Timer_t struct with an option to start it on initialization.
 * @param start If 1, starts the timer on initialization.
 * @return Initialized Timer_t struct.
 */
struct c_utils_timer *c_utils_timer_init(int start);

/**
 * Start the timer.
 * @param timer Timer to start.
 * @return 1 on success.
 */
int c_utils_timer_start(struct c_utils_timer *timer);

/**
 * Stops the timer.
 * @param timer Timer to stop.
 * @return 1 on success.
 */
int c_utils_timer_stop(struct c_utils_timer *timer);

/**
 * Returns the total time in Hours:Minutes:Seconds. (I.E 00:04:30)
 * Note: If you do not stop the timer, it will produce undefined behavior.
 * @param timer Timer to obtain the total time of.
 * @return Formatted total time.
 */
char *c_utils_timer_string(struct c_utils_timer *timer);

/**
 * Destroys the passed timer.
 * @param timer Timer to be destroyed.
 */
void c_utils_timer_destroy(struct c_utils_timer *timer);

#endif /* END C_UTILS_TIMER_H */