#ifndef NET_UTILS_HELPER_H
#define NET_UTILS_HELPER_H

/* 
* As many system calls may fail due to an async signal, whether it be select, send, recv, or even close,
* a system call that fails due to EINTR should be restarted, which is why I use TEMP_FAILURE_RETRY glibc macro.
*/
#define _GNU_SOURCE 1

#include <unistd.h>
#include <MU_Logger.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

/* Client-Server general data structures below */

typedef struct {
   /// Amount of messages sent.
   atomic_size_t messages_sent;
   /// Amount of messages received.
   atomic_size_t messages_received;
   /// Total amount of data sent.
   atomic_size_t bytes_sent;
   /// Total amount of data received.
   atomic_size_t bytes_received;
} NU_Atomic_Data_t;

/// Helper to make determining buffer and file sizes a lot easier.
typedef enum {
   /// Represents a byte.
   NU_BYTE = 1,
   /// Represents a kilobyte.
   NU_KILOBYTE = 1024,
   /// Represents a megabyte
   NU_MEGABYTE = 1,048,576,
   /// Represents a gigabyte
   NU_GIGABYTE = 1,073,741,824
} NU_Data_Size_t;

/**
 * @brief Wraps a bound socket on a given port.
 * 
 * This type keeps track of the sockfd that was created from socket() and bind(), and is used
 * when attempting to accept a new conn on the socket encapsulated within. It keeps track of
 * the port it is bound to, as well as function as a list for multiple bound socket. 
 * 
 * This type is reusable, and is good for if you wish to create more than one socket, as it helps
 * make managing them easier. Also, if you happen to unbind or shutdown a server without destroying it,
 * as an example, and wish to rebind it to new ports, these types will be recycled, minimizing the overhead
 * of creation.
 * 
 * This type must NOT be freed, as it will cause undefined behavior; instead, it is freed when the server is destroyed.
 */
typedef struct NU_Bound_Socket_t{
   /// The bound socket.
   volatile int sockfd;
   /// Port the socket is bound to.
   unsigned int port;
   /// RWLock to ensure thread safety.
   pthread_rwlock_t *lock;
   /// Flag to determine if it is bound.
   volatile unsigned char is_bound;
} NU_Bound_Socket_t;

/// Locks the rwlock iff not NULL.
void NU_lock_rdlock(pthread_rwlock_t *lock);

void NU_lock_wrlock(pthread_rwlock_t *lock);

void NU_unlock_rwlock(pthread_rwlock_t *lock);

void NU_lock_mutex(pthread_mutex_t *lock);

void NU_unlock_mutex(pthread_mutex_t *lock);

size_t NU_send_all(int sockfd, const void *buf, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

size_t NU_timed_receive(int sockfd, void *buf, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

int NU_timed_accept(int sockfd, char **ip_addr, unsigned int timeout, MU_Logger_t *logger);

// Implement. Note to self: Needs to externally locked before calling.
NU_Connection_t *NU_reuse_connection(NU_Connection_t **connections, MU_Logger_t *logger);

// Implement
NU_Connection_t **NU_select_receive_connections(NU_Connect_t **connections, size_t *size, unsigned int timeout, MU_Logger_t *logger);

// Implement
NU_Connection_t **NU_select_send_connections(NU_Connect_t **connections, size_t *size, unsigned int timeout, MU_Logger_t *logger);

// Implement
char *NU_Atomic_Data_to_string(NU_Atomic_Data_t *data);

void NU_Atomic_Data_increment_received(NU_Atomic_Data_t *data, unsigned int bytes);

void NU_Atomic_Data_increment_sent(NU_Atomic_Data_t *data, unsigned int bytes);

NU_Atomic_Data_t *NU_Atomic_Data_create(void);

int NU_is_selected(int flags, int mask);

#endif /* END NET_UTILS_HELPER_H */