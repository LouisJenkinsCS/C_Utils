#ifndef NET_UTILS_HELPER_H
#define NET_UTILS_HELPER_H

#define _GNU_SOURCE 1

#include <unistd.h>
#include <Misc_Utils.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

/// NUH_* namespace reserved for Net Utils Helper functions.

/// Default action
#define NU_NONE 1 << 0
/// Creates the socket as UDP
#define NU_UDP 1 << 1
/// Set socket options to enable broadcasting. Will also flag as UDP.
#define NU_BROADCAST 1 << 2
/// Writes data to a binary file.
#define NU_BINARY 1 << 3

/* Client-Server general data structures below */

typedef struct {
   size_t messages_sent;
   /// Amount of messages received.
   size_t messages_received;
   /// Total amount of data sent.
   size_t bytes_sent;
   /// Total amount of data received.
   size_t bytes_received;
} NU_Collective_Data_t;

typedef struct {
   /// Container for buffer.
   char *buffer;
   /// Current size of buffer.
   size_t size;
} NU_Bounded_Buffer_t;

int NUH_resize_buffer(NU_Bounded_Buffer_t *bbuf, size_t new_size, MU_Logger_t *logger);
 
size_t NUH_send_all(int sockfd, const char *message, unsigned int timeout, MU_Logger_t *logger);

size_t NUH_timed_receive(int sockfd, NU_Bounded_Buffer_t *bbuf, unsigned int timeout, MU_Logger_t *logger);

int NUH_get_socket(struct addrinfo **results, MU_Logger_t *logger);

int NUH_timed_accept(int sockfd, char **ip_addr, unsigned int timeout, MU_Logger_t *logger);

char *NUH_data_to_string(NU_Collective_Data_t data);

int NUH_is_selected(int flags, int mask);

#endif /* END NET_UTILS_HELPER_H */