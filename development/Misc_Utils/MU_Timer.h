#ifndef MU_TIMER_H
#define MU_TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    /// Keeps track of the start time.
	time_t *start;
    /// Keeps track of the end time.
	time_t *end;
	/// Whether or not the timer is started, to prevent undefined behavior if it isn't.
	volatile unsigned char is_running;
	/// Whether or not the timer has been finished, to prevent undefined behavior if it isn't.
	volatile unsigned char is_finished;
} MU_Timer_t;

/**
 * Creates a new Timer_t struct with an option to start it on initialization.
 * @param start If 1, starts the timer on initialization.
 * @return Initialized Timer_t struct.
 */
MU_Timer_t *MU_Timer_Init(int start);

/**
 * Start the timer.
 * @param timer Timer to start.
 * @return 1 on success.
 */
int MU_Timer_Start(MU_Timer_t *timer);

/**
 * Stops the timer.
 * @param timer Timer to stop.
 * @return 1 on success.
 */
int MU_Timer_Stop(MU_Timer_t *timer);

/**
 * Returns the total time in Hours:Minutes:Seconds. (I.E 00:04:30)
 * Note: If you do not stop the timer, it will produce undefined behavior.
 * @param timer Timer to obtain the total time of.
 * @return Formatted total time.
 */
char *MU_Timer_To_String(MU_Timer_t *timer);

/**
 * Destroys the passed timer.
 * @param timer Timer to be destroyed.
 */
void MU_Timer_Destroy(MU_Timer_t *timer);

#endif /* END MU_TIMER_H */