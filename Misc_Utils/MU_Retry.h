#ifndef MU_RETRY_H
#define MU_RETRY_H

#include <errno.h>

#define MU_TEMP_FAILURE_RETRY(storage, function) do { \
	errno = 0; \
	while((storage = function),  errno == EINTR); \
} while(0)


#endif /* endif MU_RETRY_H */