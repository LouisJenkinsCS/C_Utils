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

#ifdef C_UTILS_USE_POSIX_STD
#define DEBUG(...) MU_DEBUG(__VA_ARGS__)
#define ASSERT(...) MU_ASSERT(__VA_ARGS__)
#define LOG_TRACE(...) MU_LOG_TRACE(__VA_ARGS__)
#define LOG_VERBOSE(...) MU_LOG_VERBOSE(__VA_ARGS__)
#define LOG_INFO(...) MU_LOG_INFO(__VA_ARGS__)
#define LOG_CUSTOM(...) MU_LOG_CUSTOM(__VA_ARGS__)
#define LOG_EVENT(...) MU_LOG_EVENT(__VA_ARGS__)
#define LOG_WARNING(...) MU_LOG_WARNING(__VA_ARGS__)
#define LOG_ERROR(...) MU_LOG_ERROR(__VA_ARGS__)
#define LOG_ASSERT(...) MU_LOG_ASSERT(__VA_ARGS__)
#define LOGGER_AUTO_CREATE(...) MU_LOGGER_AUTO_CREATE(__VA_ARGS__)
#define logger_t MU_Logger_t
#define logger_create(...) MU_Logger_create(__VA_ARGS__)
#define logger_destroy(...) MU_Logger_destroy(__VA_ARGS__)
#endif

/**
 * A simple logging utility that supports various log levels, accurate logging of
 * location it is called, time and date and even supports custom logging format.
 */

/**
 * @brief The logger level associated with each logging macro.
 * 
 * The logger level determines precisely what gets printed, as any log macros that's minimum-requirement is below
 * the logger's level, will not log a message. For instance, if you select MU_ALL, then all messages are displayed, however
 * if you choose MU_ERROR, then only ERROR and ASSERT messages are displayed. MU_CUSTOM allows the user to define the prefix
 * to appear inside of the brackets, and also does not contain the file and line number.
 */
typedef enum {
	/// Display all log levels.
	MU_ALL = 0,
	/// Display Trace, Verbose, Info, Custom, Events, Warnings, Errors and Assertions only.
	MU_TRACE,
	/// Display Verbose, Info, Custom, Events, Warnings, Errors and Assertions only.
	MU_VERBOSE,
	/// Display Info, Custom, Events, Warnings, Errors and Assertions only.
	MU_INFO,
	/// Display Custom, Events, Warnings, Errors and Assertions only.
	MU_CUSTOM,
	/// Display Events, Warnings, Errors and Assertions only.
	MU_EVENT,
	/// Display Warnings, Errors and Assertions only.
	MU_WARNING,
	/// Display Errors and Assertions only.
	MU_ERROR,
	/// Display Assertions only.
	MU_ASSERTION,
	/// Does not display any Warnings, Errors, or even Assertions.
	MU_NONE
} MU_Logger_Level_e;

typedef struct {
	char *all_f;
	char *trace_f;
	char *verbose_f;
	char *info_f;
	char *custom_f;
	char *event_f;
	char *warning_f;
	char *error_f;
	char *assertion_f;
	char *default_f;
} MU_Logger_Format_t;

/**
 * @brief Logger utility to log various types of messages according to different MU_Logger_Level_t levels.
 * 
 * The base structure for the logger, contains the logging formats, it's current log level,
 * and the file it is logging to.
 */
typedef struct {
	/// The log file to write to.
	FILE *file;
	/// The formatter for log strings.
	MU_Logger_Format_t format;
	/// The level determining what messages will be printed.
	MU_Logger_Level_e level;
} MU_Logger_t;

#ifdef NDEBUG
/// If NDEBUG is defined, then MU_DEBUG becomes a NOP.
#define MU_DEBUG(message, ...)
#else
/// Prints a debug message, along with it's timestamp, file and line of code it's on.
#define MU_DEBUG(message, ...) do { \
	char *timestamp = MU_get_timestamp(); \
	fprintf(stderr, "%s: [DEBUG]: %s: " message "\n", timestamp, __FUNCTION__, ##__VA_ARGS__); \
	free(timestamp); \
} while(0)
#endif
/// An assertion which prints to stderr, the logfile and also shows the file and line that triggered it as well as timestamp.
#define MU_ASSERT(condition, logger, message, ...) !condition ? MU_Logger_log(logger, MU_ASSERTION, NULL, message, MU_LOGGER_STRINGIFY(condition), \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__), exit(EXIT_FAILURE) : false;

/*
	Log level of most imminent priority, normally used when an allocation fails.
*/
#define MU_LOG_ASSERT(logger, message, ...) MU_Logger_log(logger, MU_ASSERTION, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level to alert than error has occured, and should be fixed immediately.
*/
#define MU_LOG_ERROR(logger, message, ...) MU_Logger_log(logger, MU_ERROR, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level to alert the developer of a potential error, but is not severe enough to count as one.
*/
#define MU_LOG_WARNING(logger, message, ...) MU_Logger_log(logger, MU_WARNING, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level for custom log messages.
*/
#define MU_LOG_CUSTOM(logger, custom_level, message, ...) MU_Logger_log(logger, MU_CUSTOM, custom_level, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level used for logging events, specifically for MU_Events, but can be used by anyone.
*/
#define MU_LOG_EVENT(logger, event, message, ...) MU_Logger_log(logger, MU_EVENT, NULL, message, event, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level meant for informational logging, meaning it's good for the user to know, highlighting progress through
	a program. You should find less of these than MU_VERBOSE, and it should contain information that is more important.
*/
#define MU_LOG_INFO(logger, message, ...) MU_Logger_log(logger, MU_INFO, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level meant for keeping track of verbose information, meaning it's not important enough for INFO, but very useful
	for debugging without it being too tedious. Won't clog the log file as much as MU_TRACE. 
*/
#define MU_LOG_VERBOSE(logger, message, ...) MU_Logger_log(logger, MU_VERBOSE, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

/*
	Log level meant for tracing information that is more verbose than MU_VERBOSE, normally tedious tracing of the 
	flow of a program.
*/
#define MU_LOG_TRACE(logger, message, ...) MU_Logger_log(logger, MU_TRACE, NULL, message, NULL, \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define MU_LOGGER_STRINGIFY(x) MU_LOGGER_STRINGIFY_THIS(x)
#define MU_LOGGER_STRINGIFY_THIS(x) #x

#define MU_LOGGER_AUTO_CREATE(logger, filename, mode, level) \
__attribute__((constructor)) static void init_ ##logger (void){ \
	logger = MU_Logger_create(filename, mode, level); \
} \
\
__attribute__((destructor)) static void destroy_ ##logger (void){ \
	MU_Logger_destroy(logger); \
} \

/**
 * Creates a new logger with the given file name and mode, and only log beyond level.
 * @param filename Name of the file.
 * @param mode Mode of the file.
 * @param level Level to log up to, I.E MU_INFO logs only INFO and above.
 * @return Logger, or null if allocation error.
 */
MU_Logger_t *MU_Logger_create(const char *filename, const char *mode, MU_Logger_Level_e level);

/**
 * Returns the format for the log level, and if not initialized, returns all, 
 * then falls back to default if not specified either.
 * @param logger
 * @param level
 * @return 
 */
const char *MU_Logger_Format_get(MU_Logger_t *logger, MU_Logger_Level_e level);

/**
 * Returns stringification of log level.
 * @param level Log level.
 * @return Stringified log level.
 */
const char *MU_Logger_Level_to_string(MU_Logger_Level_e level);

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
bool MU_Logger_log(MU_Logger_t *logger, MU_Logger_Level_e level, const char *custom_level, const char *msg, const char *cond, const char *file_name, const char *line_number, const char *function_name, ...);

/**
 * Destroys the logger.
 * @param logger
 * @return 
 */
bool MU_Logger_destroy(MU_Logger_t *logger);

/**
 * Gets the current timestamp.
 * @return 
 */
char *MU_get_timestamp(void);

#endif