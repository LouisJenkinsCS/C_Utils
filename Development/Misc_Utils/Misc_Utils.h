#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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

typedef enum {
	/// Display all types of warnings.
	MU_ALL,
	/// Display Info, Warnings, Errors and Assertions only.
	MU_INFO,
	/// Display Warnings, Errors and Assertions only.
	MU_WARNING,
	/// Display Errors and Assertions only.
	MU_ERROR,
	/// Does not display any Warnings, Errors, or even Assertions.
	MU_NONE
} MU_Logger_Level_t;

typedef struct {
	/// The log file to write to.
	FILE *file;
	/// The level determining what messages will be printed.
	MU_Logger_Level_t level;
	/// The reference count, to allow static declarations easier. 
	volatile size_t reference_count;
	/// Determines whether or not it's initialized.
	volatile unsigned char is_initialized;
	/// Lock used to ensure only one thread dereferences the count safely.
	pthread_mutex_t *decrement_count;
} MU_Logger_t;

void MU_Logger_Init(MU_Logger_t logger, char *filename, char *mode, MU_Logger_Level_t level);

MU_Logger_t *MU_Logger_Deref(MU_Logger_t *logger);

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
#define MU_ASSERT(condition, logger) do { \
	if(!(condition)){ \
		if(logger->level > MU_ERROR) break; \
		char *timestamp = Misc_Utils_Get_Timestamp(); \
		MU_DEBUG("Assertion Failed! See log!\n"); \
		fprintf(logger->file, "%s: [ASSERT](%s:%d) An Assertion for '" #condition "' has failed!\n", Misc_Utils_Get_Timestamp(), __FILE__, __LINE__); \
		fflush(logger->file); \
		free(timestamp); \
		exit(EXIT_FAILURE); \
	} \
} while(0)
/// An assertion which prints to stderr, the logfile and returns.
#define MU_ASSERT_RETURN(condition, logger, retval) do { \
	if(!(condition)){ \
		if(logger->level > MU_ERROR) break; \
		char *timestamp = Misc_Utils_Get_Timestamp(); \
		MU_DEBUG("Assertion Failed! See log!\n"); \
		fprintf(logger->file, "%s: [ASSERT](%s:%d) An Assertion for '" #condition "' has failed!\n", Misc_Utils_Get_Timestamp(), __FILE__, __LINE__); \
		fflush(logger->file); \
		free(timestamp); \
		return retval; \
	} \
} while(0)
/// Log an error message along with timestamp, file and line of code.
#define MU_LOG_ERROR(logger, message, ...) do { \
	if(logger->level > MU_ERROR) break; \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(logger->file, "%s: [ERROR](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0) 
/// Log a warning message along with timestamp, file and line of code.
#define MU_LOG_WARNING(logger, message, ...) do { \
	if(logger->level > MU_WARNING) break; \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(logger->file, "%s: [WARNING](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)
/// Log an info message along with timestamp, file and line of code.
#define MU_LOG_INFO(logger, message, ...) do { \
	if(logger->level > MU_INFO) break; \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(logger->file, "%s: [INFO](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)
/// Log a verbose message along with timestamp, file and line of code. This differs from INFO in that it is used for
/// logging almost trivial information, good for if you want to log every single thing without having to remove it later.
#define MU_LOG_VERBOSE(logger, message, ...) do { \
	if(logger->level > MU_ALL) break; \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(logger->file, "%s: [VERBOSE](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)

typedef struct {
    /// Keeps track of the start time.
	time_t *start;
    /// Keeps track of the end time.
	time_t *end;
	/// Whether or not the timer is started, to prevent undefined behavior if it isn't.
	volatile unsigned char is_running;
	/// Whether or not the timer has been finished, to prevent undefined behavior if it isn't.
	volatile unsigned char is_finished;
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