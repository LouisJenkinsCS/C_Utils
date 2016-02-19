#ifndef MU_RETRY_H
#define MU_RETRY_H

#include <errno.h>

/**
 * @param storage
 * @param function
 * @return
 */
#define MU_TEMP_FAILURE_RETRY(storage, function) while(errno = 0, (storage = function),  errno == EINTR); 


#endif /* endif MU_RETRY_H */