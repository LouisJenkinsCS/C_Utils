#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <Misc_Utils.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

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

/* Helper functions defined below! */

static int resize_buffer(NU_Bounded_Buffer_t *bbuf, size_t new_size){
   if(!bbuf->buffer){
      bbuf->buffer = calloc(1, new_size);
      bbuf->size = bbuf->index = 0;
      MU_LOG_VERBOSE(logger, "Bounded buffer was allocated to size: %d\n", new_size);
      return 1;
   }
   if(bbuf->size == new_size) return 1;
   bbuf->buffer = realloc(bbuf->buffer, new_size);
   if(bbuf->index > new_size) {
      MU_LOG_VERBOSE(logger, "The bounded buffer's index was moved from %d to %d!\n", bbuf->index, new_size - 1);
      bbuf->index = new_size - 1;
   }
   MU_LOG_VERBOSE(logger, "The bounded buffer's size is being increased from %d to %d!\n", bbuf->size, new_size);
   bbuf->size = new_size;
   return 1;
}

static int reset_client(NU_Client_t *client){
   const int hostname_length = 100, port_length = 5;
   memset(client->hostname, '\0', hostname_length);
   memset(client->port, '\0', port_length);
   memset(client->bbuf, '\0', sizeof(NU_Bounded_Buffer_t));
   client->data->messages_sent = client->data->messages_received = client->data->bytes_sent = client->data->bytes_received = client->is_connected = 0;
   return 1;
}

static size_t send_all(int sockfd, char *message, unsigned int timeout){
   size_t buffer_size = strlen(message), total_sent = 0, data_left = buffer_size;
   int retval;
   struct timeval tv;
   fd_set can_send, can_send_copy;
   tv.tv_sec = timeout;
   tv_tv_usec = 0;
   FD_ZERO(&can_send);
   FD_SET(sockfd, &can_send);
   while(buffer_size > total_sent){
      can_send_copy = can_send;
      // Restart timeout.
      tv.tv_sec = timeout;
      if((retval = TEMP_FAILURE_RETRY(select(sockfd+1, NULL, &can_send_copy, NULL, &tv))) <= 0){
         if(!retval) MU_LOG_INFO(logger, "select: \"timed out\"\n");
         else MU_LOG_ERROR(logger, "select: \"%s\"", strerror(retval));
         break;
      }
      if((retval = send(sockfd, message[total_sent], data_left, 0)) <= 0){
         if(!retval) MU_LOG_INFO(logger, "send: \"disconnected from the stream\"\n");
         else MU_LOG_ERROR(logger, "send: \"%s\"\n", strerror(retval));
         break;
      }
      total_sent += retval;
      data_left -= retval;
   }
   return total_sent;
}

static size_t receive_all(int sockfd, NU_Bounded_Buffer_t *bbuf, unsigned int timeout){
   size_t total_received = 0, data_left = bbuf->size;
   int retval;
   struct timeval tv;
   fd_set can_receive, can_receive_copy;
   tv.tv_sec = timeout;
   tv_tv_usec = 0;
   FD_ZERO(&can_receive);
   FD_SET(sockfd, &can_receive);
   while(bbuf->size > total_received){
      can_receive_copy = can_receive;
      // After every iteration, timeout is reset.
      tv.tv_sec = timeout;
      if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &can_receive_copy, NULL, NULL, &tv))) <= 0){
         if(!retval) MU_LOG_INFO(logger, "select: \"timed out\"\n");
         else MU_LOG_ERROR(logger, "select: \"%s\"", strerror(retval));
         break;
      }
      if((retval = recv(sockfd, bbuf->buffer[bbuf->index], data_left, 0)) <= 0){
         if(!retval) MU_LOG_INFO(logger, "recv: disconnected from the stream!\n");
         else MU_LOG_ERROR(logger, "recv: \"%s\"\n", strerror(retval));
         break;
      }
      total_received += retval;
      bbuf->index += retval;
      data_left -= retval;
   }
   return total_received;
}

static int get_client_socket(struct addrinfo **results){
   struct addrinfo *current = NULL;
   int sockfd = 0, iteration = 0;
   for(current = results; current; current = current->ai_next){
      if((sockfd = socket(current->ai_family, current->ai_socktype, current->ai_protocol)) == -1) {
         MU_LOG_VERBOSE(logger, "Skipped result with error \"%s\": Iteration #%d\n", strerror(-1), ++iteration);
         continue;
      }
      MU_LOG_VERBOSE(logger, "Obtained a socket from a result: Iteration #%d\n", ++i);
      if(connect(sockfd, current->ai_addr, current->ai_addrlen) == -1){
         close(sockfd);
         MU_LOG_VERBOSE(logger, "Unable to connect to socket with error \"%s\": Iteration #%d\n", strerror(-1), ++iteration);
         continue;
      }
      break;
   }
   if(!current) return -1;
   return sockfd;
}

#endif /* END NET_UTILS_H */