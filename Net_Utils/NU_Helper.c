#include <NU_Helper.h>
 
size_t NU_send_all(int sockfd, const void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
   size_t total_sent = 0, data_left = buf_size;
   long long int retval;
   struct timeval tv;
   fd_set can_send, can_send_copy;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_send);
   FD_SET(sockfd, &can_send);
   while(buf_size > total_sent){
      can_send_copy = can_send;
      // Restart timeout.
      tv.tv_sec = timeout;
      if((retval = TEMP_FAILURE_RETRY(select(sockfd+1, NULL, &can_send_copy, NULL, &tv))) <= 0){
         if(!retval) MU_LOG_INFO(logger, "NU_send_all->select: \"timed out\"\n");
         else MU_LOG_ERROR(logger, "NU_send_all->select: \"%s\"", strerror(errno));
         break;
      }
      if((retval = TEMP_FAILURE_RETRY(send(sockfd, buffer + total_sent, data_left, 0))) <= 0){
         if(!retval) MU_LOG_INFO(logger, "NU_send_all->send: \"Disconnected from the stream\"\n");
         else MU_LOG_ERROR(logger, "NU_send_all->send: \"%s\"\n", strerror(errno));
         break;
      }
      total_sent += retval;
      data_left -= retval;
   }
   return total_sent;
}

size_t NU_timed_receive(int sockfd, void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
   long long int retval;
   struct timeval tv;
   fd_set can_receive;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_receive);
   FD_SET(sockfd, &can_receive);
   if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &can_receive, NULL, NULL, &tv))) <= 0){
      if(!retval) MU_LOG_INFO(logger, "NU_timed_receive->select: \"Timed out!\"\n");
      else MU_LOG_ERROR(logger, "NU_timed_receive->select: \"%s\"", strerror(errno));
      return 0;
   }
   if((retval = TEMP_FAILURE_RETRY(recv(sockfd, buffer, buf_size, 0))) <= 0){
      if(!retval) MU_LOG_INFO(logger, "NU_timed_receive->recv: \"Disconnected from the stream!\"\n");
      else MU_LOG_ERROR(logger, "NU_timed_receive->recv: \"%s\"\n", strerror(errno));
      return 0;
   }
   return retval;
}

int NU_timed_accept(int sockfd, char *ip_addr, unsigned int timeout, MU_Logger_t *logger){
   long long int retval;
   fd_set can_accept;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_accept);
   FD_SET(sockfd, &can_accept);
   if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &can_accept, NULL, NULL, &tv))) <= 0){
      if(!retval) MU_LOG_INFO(logger, "NU_timed_accept->select: \"timeout\"\n");
      else MU_LOG_ERROR(logger, "NU_timed_accept->select: \"%s\"\n", strerror(errno));
      return 0;
   }
   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   if((retval = TEMP_FAILURE_RETRY(accept(sockfd, (struct sockaddr *)&addr, &size))) == -1){
      MU_LOG_ERROR(logger, "NU_timed_accept->accept: \"%s\"\n", strerror(errno));
      return 0;
   }
   if(ip_addr){
      if(!inet_ntop(AF_INET, &addr, ip_addr , INET_ADDRSTRLEN)) MU_LOG_WARNING(logger, "inet_ntop: \"%s\"\n", strerror(errno));
   }
   return retval;
}

char *NU_Atomic_Data_to_string(NU_Atomic_Data_t *data){
   char *data_str;
   asprintf(&data_str, "messages_sent: %zu, bytes_sent: %zu, messages_received: %zu, bytes_received: %zu",
      data->messages_sent, data->bytes_sent, data->messages_received, data->bytes_received);
   return data_str;
}

void NU_Atomic_Data_increment_received(NU_Atomic_Data_t *data, unsigned int bytes){
   atomic_fetch_add(&data->bytes_received, bytes);
   atomic_fetch_add(&data->messages_received, 1);
}

void NU_Atomic_Data_increment_sent(NU_Atomic_Data_t *data, unsigned int bytes){
   atomic_fetch_add(&data->bytes_sent, bytes);
   atomic_fetch_add(&data->messages_sent, 1);
}

NU_Atomic_Data_t *NU_Atomic_Data_create(void){
   NU_Atomic_Data_t *data = calloc(1, sizeof(NU_Atomic_Data_t));
   if(!data) return NULL;
   // To avoid using locks, I must initialize them as atomic.
   data->bytes_sent = ATOMIC_VAR_INIT(0);
   data->bytes_received = ATOMIC_VAR_INIT(0);
   data->messages_received = ATOMIC_VAR_INIT(0);
   data->messages_sent = ATOMIC_VAR_INIT(0);
   return data;
}

int NUH_is_selected(int flags, int mask){
   return flags & mask;
}
