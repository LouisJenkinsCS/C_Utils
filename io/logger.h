#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <assert.h>

struct c_utils_logger_t;

enum c_utils_log_level_e {
	/// Display all log levels.
	LOG_LEVEL_ALL = 0,
	/// Display Trace, Verbose, Info, Custom, Events, Warnings, Errors and Assertions only.
	LOG_LEVEL_TRACE,
	/// Display Verbose, Info, Custom, Events, Warnings, Errors and Assertions only.
	LOG_LEVEL_VERBOSE,
	/// Display Info, Custom, Events, Warnings, Errors and Assertions only.
	LOG_LEVEL_INFO,
	/// Display Custom, Events, Warnings, Errors and Assertions only.
	LOG_LEVEL_CUSTOM,
	/// Display Events, Warnings, Errors and Assertions only.
	LOG_LEVEL_EVENT,
	/// Display Warnings, Errors and Assertions only.
	LOG_LEVEL_WARNING,
	/// Display Errors and Assertions only.
	LOG_LEVEL_ERROR,
	/// Display Assertions only.
	LOG_LEVEL_ASSERTION,
	/// Does not display any Warnings, Errors, or even Assertions.
	LOG_LEVEL_NONE
};

#ifdef NO_C_UTILS_PREFIX
typedef struct c_utils_logger_t logger_t;
#define logger_create(...) c_utils_logger_create(__VA_ARGS__)
#define logger_destroy(...) c_utils_logger_destroy(__VA_ARGS__)
#endif

#ifdef NDEBUG
/// If NDEBUG is defined, then MU_DEBUG becomes a NOP.
#define DEBUG(message, ...)
#else
/// Prints a debug message, along with it's timestamp, file and line of code it's on.
#define DEBUG(message, ...) do { \
	char *timestamp = MU_get_timestamp(); \
	fprintf(stderr, "%s: [DEBUG]: %s: " message "\n", timestamp, __FUNCTION__, ##__VA_ARGS__); \
	free(timestamp); \
} while(0)
#endif
/// An assertion which prints to stderr, the logfile and also shows the file and line that triggered it as well as timestamp.
#define ASSERT(condition, logger, message, ...) !condition ? c_utils_logger_log(logger, MU_ASSERTION, NULL, message, MU_LOGGER_STRINGIFY(condition), \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__), exit(EXIT_FAILURE) : false;

/*
	Log level of most imminent priority, normally used when an allocation fails.
*/
#define LOG_ASSERT(logger, message, ...) c_utils_logger_log(logger, MU_ASSERTION, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level to alert than error has occured, and should be fixed immediately.
*/
#define LOG_ERROR(logger, message, ...) c_utils_logger_log(logger, MU_ERROR, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level to alert the developer of a potential error, but is not severe enough to count as one.
*/
#define LOG_WARNING(logger, message, ...) c_utils_logger_log(logger, MU_WARNING, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level for custom log messages.
*/
#define LOG_CUSTOM(logger, custom_level, message, ...) c_utils_logger_log(logger, MU_CUSTOM, custom_level, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level used for logging events, specifically for MU_Events, but can be used by anyone.
*/
#define LOG_EVENT(logger, event, message, ...) c_utils_logger_log(logger, MU_EVENT, NULL, message, event, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level meant for informational logging, meaning it's good for the user to know, highlighting progress through
	a program. You should find less of these than MU_VERBOSE, and it should contain information that is more important.
*/
#define LOG_INFO(logger, message, ...) c_utils_logger_log(logger, MU_INFO, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level meant for keeping track of verbose information, meaning it's not important enough for INFO, but very useful
	for debugging without it being too tedious. Won't clog the log file as much as MU_TRACE. 
*/
#define LOG_VERBOSE(logger, message, ...) c_utils_logger_log(logger, MU_VERBOSE, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level meant for tracing information that is more verbose than MU_VERBOSE, normally tedious tracing of the 
	flow of a program.
*/
#define LOG_TRACE(logger, message, ...) c_utils_logger_log(logger, MU_TRACE, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define LOGGER_STRINGIFY(x) LOGGER_STRINGIFY_THIS(x)
#define LOGGER_STRINGIFY_THIS(x) #x

#define LOGGER_AUTO_CREATE(logger, filename, mode, level) \
__attribute__((constructor)) static void init_ ##logger (void){ \
	logger = c_utils_logger_create(filename, mode, level); \
} \
\
__attribute__((destructor)) static void destroy_ ##logger (void){ \
	c_utils_logger_destroy(logger); \
} \

/**
 * Creates a new logger with the given file name and mode, and only log beyond level.
 * @param filename Name of the file.
 * @param mode Mode of the file.
 * @param level Level to log up to, I.E MU_INFO logs only INFO and above.
 * @return Logger, or null if allocation error.
 */
struct c_utils_logger_t *c_utils_logger_create(const char *filename, const char *mode, enum c_utils_log_level_e level);

/**
 * This should NEVER be called outside of the implementation for this logger. Instead, use the macros above.
 * @param logger
 * @param level
 * @param custom_level
 * @param msg
 * @param cond
 * @param file_name
 * @param line_number
 * @param function_name
 * @param ...
 * @return 
 */
bool c_utils_logger_log(struct c_utils_logger_t *logger, enum c_utils_log_level_e level, const char *custom_level, const char *msg, const char *cond, const char *file_name, const char *line_number, const char *function_name, ...);

/**
 * Destroys the logger.
 * @param logger
 * @return 
 */
bool c_utils_logger_destroy(struct c_utils_logger_t *logger);

#endif