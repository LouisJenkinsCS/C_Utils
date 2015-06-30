#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
	MU_ALL = 0,
	/// Display Info, Custom, Warnings, Errors and Assertions only.
	MU_INFO,
	/// Display Custom, Warnings, Errors and Assertions only.
	MU_CUSTOM,
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
} MU_Logger_t;

#ifdef NDEBUG
/// If NDEBUG is defined, then MU_DEBUG becomes a NOP.
#define MU_DEBUG(message, ...)
#else
/// Prints a debug message, along with it's timestamp, file and line of code it's on.
#define MU_DEBUG(message, ...) do { \
	char *timestamp = MU_Get_Timestamp(); \
	fprintf(stderr, "%s: [DEBUG]: " message "\n", timestamp, ##__VA_ARGS__); \
	free(timestamp); \
} while(0)
#endif
/// An assertion which prints to stderr, the logfile and also shows the file and line that triggered it as well as timestamp.
#define MU_ASSERT(condition, logger, message, ...) do { \
	if(!(condition)){ \
		if(!logger || !logger->file || logger->level > MU_ERROR) assert(condition); \
		char *timestamp = MU_Get_Timestamp(); \
		MU_DEBUG("Assertion Failed! See log!\n"); \
		fprintf(logger->file, "%s: [ASSERT](%s:%d) Condition: \"" #condition "\"; Message: " #message "\n", MU_Get_Timestamp(), __FILE__, __LINE__, ##__VA_ARGS__); \
		fflush(logger->file); \
		free(timestamp); \
		exit(EXIT_FAILURE); \
	} \
} while(0)
/// An assertion which prints to stderr, the logfile and returns.
#define MU_ASSERT_RETURN(condition, logger, retval, message, ...) do { \
	if(!(condition)){ \
		if(!logger || !logger->file || logger->level > MU_ERROR) return retval; \
		char *timestamp = MU_Get_Timestamp(); \
		MU_DEBUG("Assertion Failed! See log!\n"); \
		fprintf(logger->file, "%s: [ASSERT](%s:%d) Condition: \"" #condition "\"; Message: " #message "\n", MU_Get_Timestamp(), __FILE__, __LINE__, ##__VA_ARGS__); \
		fflush(logger->file); \
		free(timestamp); \
		return retval; \
	} \
} while(0)
/// Log an error message along with timestamp, file and line of code.
#define MU_LOG_ERROR(logger, message, ...) do { \
	if(!logger || !logger->file || logger->level > MU_ERROR) break; \
	char *timestamp = MU_Get_Timestamp(); \
	fprintf(logger->file, "%s: [ERROR](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0) 
/// Log a warning message along with timestamp, file and line of code.
#define MU_LOG_WARNING(logger, message, ...) do { \
	if(!logger || !logger->file || logger->level > MU_WARNING) break; \
	char *timestamp = MU_Get_Timestamp(); \
	fprintf(logger->file, "%s: [WARNING](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)
/// Log a customer message along with timestamp, without file and line of code.
#define MU_LOG_CUSTOM(logger, prefix, message, ...) do { \
	if(!logger || !logger->file || logger->level > MU_CUSTOM) break; \
	char *timestamp = MU_Get_Timestamp(); \
	fprintf(logger->file, "%s: [%s]: " message "\n", timestamp, prefix, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)
/// Log an info message along with timestamp, file and line of code.
#define MU_LOG_INFO(logger, message, ...) do { \
	if(!logger || !logger->file || logger->level > MU_INFO) break; \
	char *timestamp = MU_Get_Timestamp(); \
	fprintf(logger->file, "%s: [INFO](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)
/// Log a verbose message along with timestamp, file and line of code. This differs from INFO in that it is used for
/// logging almost trivial information, good for if you want to log every single thing without having to remove it later.
#define MU_LOG_VERBOSE(logger, message, ...) do { \
	if(!logger || !logger->file || logger->level > MU_ALL) break; \
	char *timestamp = MU_Get_Timestamp(); \
	fprintf(logger->file, "%s: [VERBOSE](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	fflush(logger->file); \
	free(timestamp); \
} while(0)


/**
 * @brief Initialize a logger passed to it.
 * 
 * Will initialize a logger passed to it, as long as logger, filename and the mode passed
 * are not NULL. Hence, logger must be allocated before being passed to this function.
 * If the filename or mode are invalid, and file isn't able to be created, then it returns 0
 * for failure. The level passed determines whether or not the log macros actually log the
 * message to file; if the level is not greater than the macro's level, it will log it to file.
 * 
 * @param logger Logger to Initialize.
 * @param filename Name of the log file to be created.
 * @param mode Mode to open the file in, I.E 'w', 'r', 'rw'.
 * @param level The minimum level of logging to be processed. Anything below it will be ignored.
 * @return 1 if successful, 0 if logger, filename or mode are NULL or if unable to open the file.
 */
int MU_Logger_Init(MU_Logger_t *logger, const char *filename, const char *mode, MU_Logger_Level_t level);

/**
 * @brief Destroys the logger passed to it, freeing it if flagged.
 * @param logger Logger to be destroyed.
 * @param free_ptr Flag for whether or not the logger gets freed.
 * @return 1 on success, 0 if logger is NULL.
 */
int MU_Logger_Destroy(MU_Logger_t *logger, unsigned int free_ptr);

/**
 * Obtain the current timestamp in Hours:Minutes:Seconds AM/PM. (I.E 11:45:30 AM)
 * @return Formatted timestamp.
 */
char *MU_Get_Timestamp(void);

#endif