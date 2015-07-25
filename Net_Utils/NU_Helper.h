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

/* Client-Server general data structures below */

typedef struct {
   /// Amount of messages sent.
   _Atomic size_t messages_sent;
   /// Amount of messages received.
   _Atomic size_t messages_received;
   /// Total amount of data sent.
   _Atomic size_t bytes_sent;
   /// Total amount of data received.
   _Atomic size_t bytes_received;
} NU_Atomic_Data_t;

/// Helper to make determining buffer and file sizes a lot easier.
typedef enum {
   /// Represents a byte.
   NU_BYTE = 1,
   /// Represents a kilobyte.
   NU_KILOBYTE = 1024,
   /// Represents a megabyte
   NU_MEGABYTE = 1048576,
   /// Represents a gigabyte
   NU_GIGABYTE = 1073741824
} NU_Data_Size_t;

size_t NU_send_all(int sockfd, const void *buf, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

size_t NU_timed_receive(int sockfd, void *buf, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

bool NU_timed_accept(int sockfd, char *ip_addr, unsigned int timeout, MU_Logger_t *logger);

// Implement
char *NU_Atomic_Data_to_string(NU_Atomic_Data_t *data);

void NU_Atomic_Data_increment_received(NU_Atomic_Data_t *data, unsigned int bytes);

void NU_Atomic_Data_increment_sent(NU_Atomic_Data_t *data, unsigned int bytes);

NU_Atomic_Data_t *NU_Atomic_Data_create(void);

bool NU_is_selected(int flags, int mask);

#endif /* END NET_UTILS_HELPER_H */
