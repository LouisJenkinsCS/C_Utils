#ifndef C_UTILS_RETRY_H
#define C_UTILS_RETRY_H

#include <errno.h>

/**
 * Portable TEMP_FAILURE_RETRY macro from GCC. It will loop until errno is not EINTR, as signals can and will
 * interrupt syscalls.
 * @param storage Stores result.
 * @param function Name of syscall
 */
#define C_UTILS_TEMP_FAILURE_RETRY(storage, function) while (errno = 0, (storage = function),  errno == EINTR); 


#endif /* endif C_UTILS_RETRY_H */