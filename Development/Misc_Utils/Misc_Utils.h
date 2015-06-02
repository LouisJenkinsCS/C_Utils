#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stdio.h>
#include <time.h>

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
		fprintf(stderr, "Assertion Failed! See log!\n"); \
		fprintf(file, "%s: [ASSERT](%s:%d) An Assertion for '" #condition "' has failed!\n", Misc_Utils_Get_Timestamp(), __FILE__, __LINE__); \
		free(timestamp); \
		exit(EXIT_FAILURE); \
	} \
} while(0)
/// Log an error message along with timestamp, file and line of code.
#define MU_LOG_ERROR(file, message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(file, "%s: [ERROR](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	free(timestamp); \
} while(0) 
/// Log a warning message along with timestamp, file and line of code.
#define MU_LOG_WARNING(file, message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(file, "%s: [WARNING](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	free(timestamp); \
} while(0)
/// Log an info message along with timestamp, file and line of code.
#define MU_LOG_INFO(file, message, ...) do { \
	char *timestamp = Misc_Utils_Get_Timestamp(); \
	fprintf(file, "%s: [INFO](%s:%d) " message "\n", timestamp, __FILE__, __LINE__, ##__VA_ARGS__); \
	free(timestamp); \
} while(0)
/// Checks if a variable is true, otherwise will return. Will not log to file.
#define MU_CHECK(obj, retval, message, ...) do { \
	if(!(obj)) MU_DEBUG(message, ##__VA_ARGS__); \
	return retval; \
} while(0)

typedef struct {
	time_t *start;
	time_t *end;
} Timer_t;

Timer_t *Timer_Init(int start);

int Timer_Start(Timer_t *timer);

int Timer_Stop(Timer_t *timer);

char *Timer_To_String(Timer_t *timer);

void Timer_Destroy(Timer_t *timer);

/// Used to obtain a timestamp.
char *Misc_Utils_Get_Timestamp(void);
#endif