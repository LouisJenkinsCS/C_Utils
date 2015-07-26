#include <NU_Helper.h>
 
size_t NU_send_all(int sockfd, const void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
   size_t total_sent = 0, data_left = buf_size;
   long long int sent;
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
      MU_TEMP_FAILURE_RETRY(sent, select(sockfd+1, NULL, &can_send_copy, NULL, &tv));
      if(sent <= 0){
         if(!sent) MU_LOG_INFO(logger, "select: 'Timed out!'");
         else MU_LOG_ERROR(logger, "select: '%s'", strerror(errno));
         break;
      }
      MU_TEMP_FAILURE_RETRY(sent, send(sockfd, buffer + total_sent, data_left, 0));
      if(sent <= 0){
         if(!sent) MU_LOG_INFO(logger, "send: 'Disconnected from the stream'");
         else MU_LOG_ERROR(logger, "send: '%s'", strerror(errno));
         break;
      }
      total_sent += sent;
      data_left -= sent;
   }
   return total_sent;
}

size_t NU_timed_receive(int sockfd, void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
   long long int received;
   struct timeval tv;
   fd_set can_receive;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_receive);
   FD_SET(sockfd, &can_receive);
   MU_TEMP_FAILURE_RETRY(received, select(sockfd + 1, &can_receive, NULL, NULL, &tv));
   if(received <= 0){
      if(!received) MU_LOG_INFO(logger, "select: 'Timed out!'");
      else MU_LOG_ERROR(logger, "select: '%s'", strerror(errno));
      return 0;
   }
   MU_TEMP_FAILURE_RETRY(received, recv(sockfd, buffer, buf_size, 0));
   if(received <= 0){
      if(!received) MU_LOG_INFO(logger, "recv: 'Disconnected from the stream!'");
      else MU_LOG_ERROR(logger, "recv: '%s'", strerror(errno));
      return 0;
   }
   return received;
}

int NU_timed_accept(int sockfd, char *ip_addr, unsigned int timeout, MU_Logger_t *logger){
   int accepted;
   fd_set can_accept;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_accept);
   FD_SET(sockfd, &can_accept);
   MU_TEMP_FAILURE_RETRY(accepted, select(sockfd + 1, &can_accept, NULL, NULL, &tv));
   if(accepted <= 0){
      if(!accepted) MU_LOG_INFO(logger, "select: 'Timed out!'");
      else MU_LOG_ERROR(logger, "select: '%s'", strerror(errno));
      return 0;
   }
   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   MU_TEMP_FAILURE_RETRY(accepted, accept(sockfd, (struct sockaddr *)&addr, &size));
   if(accepted == -1){
      MU_LOG_ERROR(logger, "accept: '%s'", strerror(errno));
      return -1;
   }
   if(ip_addr){
      if(!inet_ntop(AF_INET, &addr, ip_addr , INET_ADDRSTRLEN)) MU_LOG_WARNING(logger, "inet_ntop: '%s'", strerror(errno));
   }
   return accepted;
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
