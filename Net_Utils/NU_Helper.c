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

size_t NU_timed_receive(int sockfd, const void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
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

int NU_timed_accept(int sockfd, char **ip_addr, unsigned int timeout, MU_Logger_t *logger){
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
      *ip_addr = calloc(1, INET_ADDRSTRLEN);
      if(!inet_ntop(AF_INET, &addr, *ip_addr , INET_ADDRSTRLEN)) MU_LOG_WARNING(logger, "inet_ntop: \"%s\"\n", strerror(errno));
   }
   return retval;
}

char *NU_Collective_Data_to_string(NU_Collective_Data_t data){
   char *data_str;
   asprintf(&data_str, "messages_sent: %zu, bytes_sent: %zu, messages_received: %zu, bytes_received: %zu",
      data.messages_sent, data.bytes_sent, data.messages_received, data.bytes_received);
   return data_str;
}

int NUH_is_selected(int flags, int mask){
   return flags & mask;
}

NU_Connection_t **NU_select_receive_connections(NU_Connection_t **connections, size_t *size, unsigned int timeout, MU_Logger_t *logger){
   if(!connections || !size || !*size){
      *size = 0;
      return NULL;
   }
   fd_set receive_set;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&receive_set);
   int max_fd = 0, are_ready;
   size_t i = 0, new_size = 0;
   for(;i < *size; i++){
      NU_Connection_t *conn = connections[i];
      if(!NU_Connection_is_valid(conn)) continue;
      int sockfd = NU_Connection_get_sockfd(conn);
      FD_SET(sockfd, &receive_set);
      new_size++;
      if(sockfd > max_fd) max_fd = sockfd;
   }
   if(!new_size) {
      *size = 0;
      return NULL;
   }
   if((are_ready = TEMP_FAILURE_RETRY(select(max_fd + 1, &receive_set, NULL, NULL, &tv))) <= 0){
      if(!are_ready) MU_LOG_INFO(logger, "NU_select_receive_connections->select: \"Timed out!\"\n");
      else MU_LOG_WARNING(logger, "NU_select_receive_connections->select: \"%s\"\n", strerror(errno));
      *size = 0;
      return NULL;
   }
   NU_Connection_t **ready_connections = malloc(sizeof(NU_Connection_t *) * are_ready);
   new_size = 0;
   for(i = 0;i < *size;i++) if(FD_ISSET(NU_Connections_get_sockfd(connections[i]), &receive_set)) ready_connections[new_size++] = connections[i];
   *size = new_size;
   return ready_connections;
}


NU_Connection_t **NU_select_send_connections(NU_Connect_t **connections, size_t *size, unsigned int timeout, MU_Logger_t *logger){
   if(!connections || !size || !*size){
      *size = 0;
      return NULL;
   }
   fd_set send_set;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&send_set);
   int max_fd = 0, are_ready;
   size_t i = 0, new_size = 0;
   for(;i < *size; i++){
      NU_Connection_t *conn = connections[i];
      if(!NU_Connection_in_use(conn)) continue;
      int sockfd = NU_Connection_get_sockfd(conn);
      FD_SET(sockfd, &send_set);
      new_size++;
      if(sockfd > max_fd) max_fd = sockfd;
   }
   if(!new_size) {
      *size = 0;
      return NULL;
   }
   if((are_ready = TEMP_FAILURE_RETRY(select(max_fd + 1, NULL , &send_set, NULL, &tv))) <= 0){
      if(!are_ready) MU_LOG_VERBOSE(logger, "NU_select_send_connections->select: \"Timed out!\"\n");
      else MU_LOG_WARNING(logger, "NU_select_send_connections->select: \"%s\"\n", strerror(errno));
      *size = 0;
      return NULL;
   }
   NU_Connection_t **ready_connections = malloc(sizeof(NU_Connection_t *) * are_ready);
   new_size = 0;
   for(i = 0;i < *size;i++){
      if(FD_ISSET(NU_Connection_get_sockfd(connections[i]), &send_set)){
         ready_connections[new_size++] = connections[i];
      }
   }
   *size = new_size;
   return ready_connections;
}

NU_Connection_t *NU_reuse_connection(NU_Connection_t **connections, size_t size, MU_Logger_t *logger){
   if(!connections || !size) return NULL;
   size_t i = 0;
   for(;i < size; i++){
      NU_Connection_t *conn = connections[i];
      NU_lock_wrlock(conn->lock);
      if(conn && !conn->in_use){
         conn->in_use = 1;
         NU_unlock_rwlock(conn->lock);
         return conn;
      }
      NU_unlock_rwlock(conn->lock);
   }
   return NULL;
}

void NU_lock_rdlock(pthread_rwlock_t *lock){
   if(lock){
      pthread_rwlock_rdlock(lock);
   }
}

void NU_lock_wrlock(pthread_rwlock_t *lock){
   if(lock){
      pthread_rwlock_wrlock(lock);
   }
}

void NU_unlock_rwlock(pthread_rwlock_t *lock){
   if(lock){
      pthread_rwlock_unlock(lock);
   }
}

void NU_lock_mutex(pthread_mutex_t *lock){
   if(lock){
      pthread_mutex_lock(lock);
   }
}

void NU_unlock_mutex(pthread_mutex_t *lock, MU_Logger_t *logger){
   if(lock){
      pthread_mutex_unlock(lock);
   }
}