#ifndef MU_RETRY_H
#define MU_RETRY_H

#include <errno.h>

/**
 * Portable TEMP_FAILURE_RETRY macro from GCC. It will loop until errno is not EINTR, as signals can and will
 * interrupt syscalls.
 * @param storage Stores result.
 * @param function Name of syscall
 */
#define MU_TEMP_FAILURE_RETRY(storage, function) while(errno = 0, (storage = function),  errno == EINTR); 


#endif /* endif MU_RETRY_H */