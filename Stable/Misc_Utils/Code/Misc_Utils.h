#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stdio.h>
#include <time.h>

/*
 * Miscallaneous Utilities for the C Programming Language, or Misc_Utils, is a collection,
 * albeit a small one, of utilities that aren't big enough to deserve it's own package.
 * 
 * Inside, it contains basic logging macros to log information to a file, a debugging
 * macro to output information to stderr in a prettified format. It should also be
 * noted that the logging functions also show the name of the file and the line where 
 * the macro is invoked, thanks to GCC magic.
 * 
 * Inside also is a timer, which is extremely bare bones for now, and allows you
 * to start, stop and obtain the total time in string format.
 */

#ifdef NDEBUG
/// If NDEBUG is defined, then MU_DEBUG becomes a NOP.
#define MU_DEBUG(message, ...)
#else
/// Prints a debug message, along with it's timestamp, file and line of code it's on.
#define MU_DEBUG(message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(stderr, "%s: [DEBUG]: " message "\n", timestamp, ##__VA_ARGS__); \
	free(timestamp); \
} while(0)
#endif
/// An assertion which prints to stderr, the logfile and also shows the file and line that triggered it as well as timestamp.
#define MU_ASSERT(condition, file) do { \
	if(!(condition)){ \
		char *timestamp = Misc_Utils_Get_Timestamp(); \
		MU_DEBUG("Assertion Failed! See log!\n"); \
		fprintf(file, "%s: [ASSERT](%s:%d) An Assertion for '" #condition "' has failed!\n", Misc_Utils_Get_Timestamp(), __FILE__, __LINE__); \
		fflush(file); \
		free(timestamp); \
		exit(EXIT_FAILURE); \
	} \
} while(0)
/// An assertion which prints to stderr, the logfile and returns.
#define MU_ASSERT_RETURN(condition, file, retval) do { \
	if(!(condition)){ \
		char *timestamp = Misc_Utils_Get_Timestamp(); \
		MU_DEBUG("Assertion Failed! See log!\n"); \
		fprintf(file, "%s: [ASSERT](%s:%d) An Assertion for '" #condition "' has failed!\n", Misc_Utils_Get_Timestamp(), __FILE__, __LINE__); \
		fflush(file); \
		free(timestamp); \
		return retval; \
	} \
} while(0)
/// Log an error message along with timestamp, file and line of code.
#define MU_LOG_ERROR(file, message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(file, "%s: [ERROR](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(file); \
	free(timestamp); \
} while(0) 
/// Log a warning message along with timestamp, file and line of code.
#define MU_LOG_WARNING(file, message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(file, "%s: [WARNING](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(file); \
	free(timestamp); \
} while(0)
/// Log an info message along with timestamp, file and line of code.
#define MU_LOG_INFO(file, message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(file, "%s: [INFO](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(file); \
	free(timestamp); \
} while(0)
/// Checks if a variable is true, otherwise will return. Will not log to file.
#define MU_CHECK(obj, retval, message, ...) do { \
	if(!(obj)) MU_DEBUG(message, ##__VA_ARGS__); \
	return retval; \
} while(0)

typedef struct {
    /// Keeps track of the start time.
	time_t *start;
    /// Keeps track of the end time.
	time_t *end;
} Timer_t;

/**
 * Creates a new Timer_t struct with an option to start it on initialization.
 * @param start If 1, starts the timer on initialization.
 * @return Initialized Timer_t struct.
 */
Timer_t *Timer_Init(int start);

/**
 * Start the timer.
 * @param timer Timer to start.
 * @return 1 on success.
 */
int Timer_Start(Timer_t *timer);

/**
 * Stops the timer.
 * @param timer Timer to stop.
 * @return 1 on success.
 */
int Timer_Stop(Timer_t *timer);

/**
 * Returns the total time in Hours:Minutes:Seconds. (I.E 00:04:30)
 * Note: If you do not stop the timer, it will produce undefined behavior.
 * @param timer Timer to obtain the total time of.
 * @return Formatted total time.
 */
char *Timer_To_String(Timer_t *timer);

/**
 * Destroys the passed timer.
 * @param timer Timer to be destroyed.
 */
void Timer_Destroy(Timer_t *timer);

/**
 * Obtain the current timestamp in Hours:Minutes:Seconds AM/PM. (I.E 11:45:30 AM)
 * @return Formatted timestamp.
 */
char *Misc_Utils_Get_Timestamp(void);
#endif