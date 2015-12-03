#ifndef NET_UTILS_HELPER_H
#define NET_UTILS_HELPER_H

/* 
* As many system calls may fail due to an async signal, whether it be select, send, recv, or even close,
* a system call that fails due to EINTR should be restarted, which is why I use TEMP_FAILURE_RETRY glibc macro.
*/
#define _GNU_SOURCE 1

#include <unistd.h>
#include <MU_Logger.h>
#include <MU_Cond_Locks.h>
#include <MU_Arg_Check.h>
#include <MU_Retry.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#endif /* END NET_UTILS_HELPER_H */
