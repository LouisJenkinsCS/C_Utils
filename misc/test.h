#ifndef C_UTILS_TEST_H
#define C_UTILS_TEST_H

#include "../io/logger.h"

#define C_UTILS_TEST(condition, logger, test) condition ? C_UTILS_PASSED(logger, test) : C_UTILS_FAILED(condition, logger, test)

#define C_UTILS_PASSED(logger, test) C_UTILS_LOG_INFO(logger, "Passed: %s", test)

#define C_UTILS_FAILED(condition, logger, test) C_UTILS_Logger_log(logger, C_UTILS_ASSERTION, NULL, "Failed: %s", C_UTILS_STRINGIFY(condition), \
	__FILE__, C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, test)

#endif /* endif C_UTILS_TEST_H */