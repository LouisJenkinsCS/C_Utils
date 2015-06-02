#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stdio.h>
#ifdef MU_NO_DEBUG
#define MU_DEBUG(message, ...)
#else
#define MU_DEBUG(message, ...) fprintf(stderr, "[DEBUG](%s:%d): '" message "'\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define MU_ASSERT(condition, file)(!condition ? fprintf(file, "%s: [ASSERT](%s:%d) An Assertion for '" condition "' has failed!\n", Misc_Utils_Get_Timestamp __FILE__, __LINE__) : condition)
#define MU_LOG_ERROR(file, message, ...) fprintf(file, "%s: [ERROR](%s:%d) " message "\n", Misc_Utils_Get_Timestamp, __FILE__, __LINE__, ##__VA_ARGS__)
#define MU_LOG_WARNING(file, message, ...) fprintf(file, "%s: [WARNING](%s:%d) " message "\n", Misc_Utils_Get_Timestamp, __FILE__, __LINE__, ##__VA_ARGS__)
#define MU_LOG_INFO(file, message, ...) fprintf(file, "%s: [INFO](%s:%d) " message "\n", Misc_Utils_Get_Timestamp, __FILE__, __LINE__, ##__VA_ARGS__)
#define MU_CHECK(obj, message, ...)(!obj ? MU_DEBUG(message, ##__VA_ARGS__) : obj)

char *Misc_Utils_Get_Timestamp(void);
#endif