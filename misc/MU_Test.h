#ifndef MU_TEST_H
#define MU_TEST_H

#include <MU_Logger.h>

#define MU_TEST(condition, logger, test) condition ? MU_PASSED(logger, test) : MU_FAILED(condition, logger, test)

#define MU_PASSED(logger, test) MU_LOG_INFO(logger, "Passed: %s", test)

#define MU_FAILED(condition, logger, test) MU_Logger_log(logger, MU_ASSERTION, NULL, "Failed: %s", MU_LOGGER_STRINGIFY(condition), \
	__FILE__, MU_LOGGER_STRINGIFY(__LINE__), __FUNCTION__, test)

#endif /* endif MU_TEST_H */