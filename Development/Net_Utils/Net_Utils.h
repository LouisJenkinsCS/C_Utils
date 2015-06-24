#ifndef NET_UTILS_H
#define NET_UTILS_H

#define _GNU_SOURCE 1

#include <unistd.h>
#include <Misc_Utils.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>


/// Default action
#define NU_NONE 1 << 0
/// Creates the socket as UDP
#define NU_UDP 1 << 1
/// Set socket options to enable broadcasting. Will also flag as UDP.
#define NU_BROADCAST 1 << 2

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

#endif /* END NET_UTILS_H */