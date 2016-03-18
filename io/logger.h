#ifndef C_UTILS_LOGGER_H
#define C_UTILS_LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <assert.h>

struct c_utils_logger;

struct c_utils_location {
	const char *line;
	const char *function;
	const char *file;
};

enum c_utils_log_level {
	/// Display all log levels.
	C_UTILS_LOG_LEVEL_ALL = 0,
	/// Display Trace, Verbose, Info, Custom, Events, Warnings, Errors and Assertions only.
	C_UTILS_LOG_LEVEL_TRACE,
	/// Display Verbose, Info, Custom, Events, Warnings, Errors and Assertions only.
	C_UTILS_LOG_LEVEL_VERBOSE,
	/// Display Info, Custom, Events, Warnings, Errors and Assertions only.
	C_UTILS_LOG_LEVEL_INFO,
	/// Display Custom, Events, Warnings, Errors and Assertions only.
	C_UTILS_LOG_LEVEL_CUSTOM,
	/// Display Events, Warnings, Errors and Assertions only.
	C_UTILS_LOG_LEVEL_EVENT,
	/// Display Warnings, Errors and Assertions only.
	C_UTILS_LOG_LEVEL_WARNING,
	/// Display Errors and Assertions only.
	C_UTILS_LOG_LEVEL_ERROR,
	/// Display Assertions only.
	C_UTILS_LOG_LEVEL_ASSERTION,
	/// Does not display any Warnings, Errors, or even Assertions.
	C_UTILS_LOG_LEVEL_NONE
};

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_logger logger_t;

/*
	Functions
*/

#define logger_create(...) c_utils_logger_create(__VA_ARGS__)
#define logger_destroy(...) c_utils_logger_destroy(__VA_ARGS__)

/*
	Macros
*/
#define DEBUG(...) C_UTILS_DEBUG(__VA_ARGS__)
#define ASSERT(...) C_UTILS_ASSERT(__VA_ARGS__)
#define LOG_TRACE(...) C_UTILS_LOG_TRACE(__VA_ARGS__)
#define LOG_VERBOSE(...) C_UTILS_LOG_VERBOSE(__VA_ARGS__)
#define LOG_CUSTOM(...) C_UTILS_LOG_CUSTOM(__VA_ARGS__)
#define LOG_INFO(...) C_UTILS_LOG_INFO(__VA_ARGS__)
#define LOG_EVENT(...) C_UTILS_LOG_EVENT(__VA_ARGS__)
#define LOG_WARNING(...) C_UTILS_LOG_WARNING(__VA_ARGS__)
#define LOG_ERROR(...) C_UTILS_LOG_ERROR(__VA_ARGS__)
#define LOG_ASSERT(...) C_UTILS_ASSERT(__VA_ARGS__)
#define LOGGER_AUTO_CREATE(...) C_UTILS_LOGGER_AUTO_CREATE(__VA_ARGS__)

/*
	Enumerators
*/
#define LOG_LEVEL_ALL C_UTILS_LOG_LEVEL_ALL
#define LOG_LEVEL_TRACE	C_UTILS_LOG_LEVEL_TRACE
#define LOG_LEVEL_VERBOSE C_UTILS_LOG_LEVEL_VERBOSE
#define LOG_LEVEL_INFO C_UTILS_LOG_LEVEL_INFO
#define LOG_LEVEL_CUSTOM C_UTILS_LOG_LEVEL_CUSTOM
#define LOG_LEVEL_EVENT	C_UTILS_LOG_LEVEL_EVENT
#define LOG_LEVEL_WARNING C_UTILS_LOG_LEVEL_WARNING
#define LOG_LEVEL_ERROR	C_UTILS_LOG_LEVEL_ERROR
#define LOG_LEVEL_ASSERTION	C_UTILS_LOG_LEVEL_ASSERTION
#define LOG_LEVEL_NONE C_UTILS_LOG_LEVEL_NONE
#endif

#define C_UTILS_LOCATION (struct c_utils_location) { .line = C_UTILS_STRINGIFY(__LINE__), .function = __FUNCTION__, .file = __FILE__}

#ifdef NDEBUG
/// If NDEBUG is defined, then MU_DEBUG becomes a NOP.
#define C_UTILS_DEBUG(message, ...)
#else
/// Prints a debug message, along with it's timestamp, file and line of code it's on.
#define C_UTILS_DEBUG(message, ...) do { \
	char *timestamp = c_utils_get_timestamp(); \
	fprintf(stderr, "%s: [DEBUG]: %s: " message "\n", timestamp, __FUNCTION__, ##__VA_ARGS__); \
	free(timestamp); \
} while (0)
#endif

/// An assertion which prints to stderr, the logfile and also shows the file and line that triggered it as well as timestamp.
#define C_UTILS_ASSERT(condition, logger, message, ...) \
	!condition ? c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL, message, C_UTILS_STRINGIFY(condition), \
		__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__), exit(EXIT_FAILURE) : false;

#define C_UTILS_ASSERT_AT(condition, logger, log_info, message, ...) \
	!condition ? c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL, message, C_UTILS_STRINGIFY(condition), \
		log_info.file, log_info.line, log_info.function, ##__VA_ARGS__), exit(EXIT_FAILURE) : false

/*
	Log level of most imminent priority, normally used when an allocation fails.
*/
#define C_UTILS_LOG_ASSERT(logger, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_ASSERT_AT(logger, log_info, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)


/*
	Log level to alert than error has occured, and should be fixed immediately.
*/
#define C_UTILS_LOG_ERROR(logger, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ERROR, NULL, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_ERROR_AT(logger, log_info, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ERROR, NULL, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)

/*
	Log level to alert the developer of a potential error, but is not severe enough to count as one.
*/
#define C_UTILS_LOG_WARNING(logger, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_WARNING, NULL, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_WARNING_AT(logger, log_info, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_WARNING, NULL, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)

/*
	Log level for custom log messages.
*/
#define C_UTILS_LOG_CUSTOM(logger, custom_level, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_CUSTOM, custom_level, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_CUSTOM_AT(logger, log_info, custom_level, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_CUSTOM, custom_level, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)

/*
	Log level used for logging events, specifically for C_UTILS_LOG_LEVEL_Events, but can be used by anyone.
*/
#define C_UTILS_LOG_EVENT(logger, event, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_EVENT, NULL, message, event, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_EVENT_AT(logger, log_info, event, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_EVENT, NULL, message, event, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)



/*
	Log level meant for informational logging, meaning it's good for the user to know, highlighting progress through
	a program. You should find less of these than C_UTILS_LOG_LEVEL_VERBOSE, and it should contain information that is more important.
*/
#define C_UTILS_LOG_INFO(logger, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_INFO, NULL, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_INFO_AT(logger, log_info, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_INFO, NULL, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)



/*
	Log level meant for keeping track of verbose information, meaning it's not important enough for INFO, but very useful
	for debugging without it being too tedious. Won't clog the log file as much as C_UTILS_LOG_LEVEL_TRACE. 
*/
#define C_UTILS_LOG_VERBOSE(logger, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_VERBOSE, NULL, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_VERBOSE_AT(logger, log_info, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_VERBOSE, NULL, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)



/*
	Log level meant for tracing information that is more verbose than C_UTILS_LOG_LEVEL_VERBOSE, normally tedious tracing of the 
	flow of a program.
*/
#define C_UTILS_LOG_TRACE(logger, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_TRACE, NULL, message, NULL, \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, ##__VA_ARGS__)

#define C_UTILS_LOG_TRACE_AT(logger, log_info, message, ...) c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_TRACE, NULL, message, NULL, \
	log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)



#define C_UTILS_STRINGIFY(x) C_UTILS_STRINGIFY_THIS(x)
#define C_UTILS_STRINGIFY_THIS(x) #x

#define C_UTILS_LOGGER_AUTO_CREATE(logger, filename, mode, level) \
__attribute__((constructor)) static void init_ ##logger (void) { \
	logger = c_utils_logger_create(filename, mode, level); \
} \
\
__attribute__((destructor)) static void destroy_ ##logger (void) { \
	c_utils_logger_destroy(logger); \
} \

/**
 * Creates a new logger with the given file name and mode, and only log beyond level.
 * @param filename Name of the file.
 * @param mode Mode of the file.
 * @param level Level to log up to, I.E C_UTILS_LOG_LEVEL_INFO logs only INFO and above.
 * @return Logger, or null if allocation error.
 */
struct c_utils_logger *c_utils_logger_create(const char *filename, const char *mode, enum c_utils_log_level level);

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
bool c_utils_logger_log(struct c_utils_logger *logger, enum c_utils_log_level level, const char *custom_level, const char *msg, const char *cond, const char *file_name, const char *line_number, const char *function_name, ...);

/**
 * Destroys the logger.
 * @param logger
 * @return 
 */
bool c_utils_logger_destroy(struct c_utils_logger *logger);

char *c_utils_get_timestamp();

#endif