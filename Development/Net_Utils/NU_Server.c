#include <NU_Server.h>

MU_Logger_t *logger = NULL;

#define MU_LOG_BSOCK_ERR(function, bsock) do { \
	MU_LOG_VERBOSE(logger, "bsock: port->\"%d\", sockfd->%d, has_next: %s\n", bsock->port, bsock->sockfd, bsock->next ? "True" : "False"); \
	MU_LOG_WARNING(logger, #function ": \"%s\"\n", strerror(-1)); \
	bsock->sockfd = 0; \
} while(0)

#define MU_LOG_SERVER(logger, message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for NU_Server's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "NU_Server_Log.txt", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

static int timed_accept(int sockfd, char **ip_addr, unsigned int timeout){
   fd_set can_accept;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_accept);
   FD_SET(sockfd, &can_accept);
   int retval;
   if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &can_accept, NULL, NULL, &tv))) <= 0){
      if(!retval) MU_LOG_INFO(logger, "timed_accept->select: \"timeout\"\n");
      else MU_LOG_ERROR(logger, "timed_accept->select: \"%s\"\n", strerror(retval));
      return 0;
   }
   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   if((retval = accept(sockfd, (struct sockaddr *)&addr, &size)) == -1){
      MU_LOG_ERROR(logger, "timed_accept->accept: \"%s\"\n", strerror(-1));
      return 0;
   }
   if(ip_addr){
      *ip_addr = calloc(1, INET_ADDRSTRLEN);
      if(!inet_ntop(AF_INET, &addr, *ip_addr , INET_ADDRSTRLEN)) MU_LOG_WARNING(logger, "inet_ntop: \"%s\"\n", strerror(-1));
   }
   return retval;
}

static int resize_buffer(NU_Bounded_Buffer_t *bbuf, size_t new_size){
   if(!bbuf->buffer){
      bbuf->buffer = calloc(1, new_size);
      bbuf->size = new_size;
      MU_LOG_VERBOSE(logger, "Bounded buffer was allocated to size: %d\n", new_size);
      return 1;
   }
   if(bbuf->size == new_size) return 1;
   bbuf->buffer = realloc(bbuf->buffer, new_size);
   MU_LOG_VERBOSE(logger, "The bounded buffer's size is being increased from %d to %d!\n", bbuf->size, new_size);
   bbuf->size = new_size;
   return 1;
}

static size_t send_all(int sockfd, const char *message, unsigned int timeout){
   size_t buffer_size = strlen(message), total_sent = 0, data_left = buffer_size;
   int retval;
   struct timeval tv;
   fd_set can_send, can_send_copy;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_send);
   FD_SET(sockfd, &can_send);
   while(buffer_size > total_sent){
      can_send_copy = can_send;
      // Restart timeout.
      tv.tv_sec = timeout;
      if((retval = TEMP_FAILURE_RETRY(select(sockfd+1, NULL, &can_send_copy, NULL, &tv))) <= 0){
         if(!retval) MU_LOG_INFO(logger, "send_all->select: \"timed out\"\n");
         else MU_LOG_ERROR(logger, "send_all->select: \"%s\"", strerror(retval));
         break;
      }
      if((retval = send(sockfd, message + total_sent, data_left, 0)) <= 0){
         if(!retval) MU_LOG_INFO(logger, "send_all->send: \"disconnected from the stream\"\n");
         else MU_LOG_ERROR(logger, "send_all->send: \"%s\"\n", strerror(retval));
         break;
      }
      total_sent += retval;
      data_left -= retval;
   }
   return total_sent;
}

static size_t timed_receive(int sockfd, NU_Bounded_Buffer_t *bbuf, unsigned int timeout){
   int retval;
   struct timeval tv;
   fd_set can_receive;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_receive);
   FD_SET(sockfd, &can_receive);
   // After every iteration, timeout is reset.
   tv.tv_sec = timeout;
   if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &can_receive, NULL, NULL, &tv))) <= 0){
      if(!retval) MU_LOG_INFO(logger, "receive_all->select: \"timed out\"\n");
      else MU_LOG_ERROR(logger, "receive_all->select: \"%s\"", strerror(retval));
      return 0;
   }
   if((retval = recv(sockfd, bbuf->buffer, bbuf->size, 0)) <= 0){
      if(!retval) MU_LOG_INFO(logger, "receive_all->recv: \"disconnected from the stream\"\n");
      else MU_LOG_ERROR(logger, "receive_all->recv: \"%s\"\n", strerror(retval));
      return 0;
   }
   return retval;
}

static int get_socket(struct addrinfo **results){
   struct addrinfo *current = NULL;
   int sockfd = 0, iteration = 0;
   for(current = *results; current; current = current->ai_next){
      if((sockfd = socket(current->ai_family, current->ai_socktype, current->ai_protocol)) == -1) {
         MU_LOG_VERBOSE(logger, "Skipped result with error \"%s\": Iteration #%d\n", strerror(-1), ++iteration);
         continue;
      }
      MU_LOG_VERBOSE(logger, "Obtained a socket from a result: Iteration #%d\n", ++iteration);
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

/* Server-specific helper functions */

static NU_Client_Socket_t *reuse_existing_client(NU_Client_Socket_t *head){
	NU_Client_Socket_t *tmp_client = NULL;
	for(tmp_client = head; tmp_client; tmp_client = tmp_client->next){
		if(!tmp_client->sockfd) break;
	}
	MU_LOG_VERBOSE(logger, "Currently existing client?: %s\n", tmp_client ? "True" : "False");
	return tmp_client;
}

static int setup_bound_socket(NU_Bound_Socket_t *bsock, unsigned int port, size_t queue_size){
	int i = 0, flag = 1;
	struct sockaddr_in my_addr;
	bsock->port = port;
	if((bsock->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		MU_LOG_BSOCK_ERR(socket, bsock);
		return 0;
	}
	if(setsockopt(bsock->sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
		shutdown(bsock->sockfd, SHUT_RDWR);
		MU_LOG_BSOCK_ERR(setsockopt, bsock);
		return 0;
	}	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(bsock->port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(bsock->sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		shutdown(bsock->sockfd, SHUT_RDWR);
		MU_LOG_BSOCK_ERR(bind, bsock);
		return 0;
	}
	if(listen(bsock->sockfd, queue_size) == -1){
		shutdown(bsock->sockfd, SHUT_RDWR);
		MU_LOG_BSOCK_ERR(listen, bsock);
		return 0;
	}
	return 1;
}

static NU_Bound_Socket_t *reuse_existing_socket(NU_Bound_Socket_t *head){
	NU_Bound_Socket_t *tmp_bsock = NULL;
	for(tmp_bsock = head; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(!tmp_bsock->sockfd) break;
	}
	MU_LOG_VERBOSE(logger, "Currently existing bsock?: %s\n", tmp_bsock ? "True" : "False");
	return tmp_bsock;
}

static void destroy_bound_socket(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	shutdown(bsock->sockfd, SHUT_RDWR);
	// If a bound socket is being destroyed, then the list of bound sockets must be updated.
	NU_Bound_Socket_t *tmp_bsock = NULL;
	if(server->sockets == bsock){ 
		server->sockets = bsock->next;
		free(bsock);
		return;
	}
	for(tmp_bsock = server->sockets; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(tmp_bsock->next == bsock) {
			tmp_bsock->next = bsock->next;
			break;
		}
	}
	if(!tmp_bsock) MU_LOG_ERROR(logger, "The bsock to be deleted was not found in the list!\n");
	free(bsock);
}

NU_Server_t *NU_Server_create(){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	return server;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, unsigned int port, size_t queue_size, int flags){
	if(!server || !port || !queue_size) return NULL;
	int is_reused = 1;
	NU_Bound_Socket_t *bsock = reuse_existing_socket(server->sockets);
	if(!bsock){
		bsock = calloc(1, sizeof(NU_Bound_Socket_t));
		MU_ASSERT_RETURN(bsock, logger, NULL, "Was unable to allocate memory for bsock!\n");
		is_reused--;
	}
	if(!is_reused){
		if(!server->sockets) server->sockets = bsock;
		else {
			NU_Bound_Socket_t *tmp_bsock = NULL;
			for(tmp_bsock = server->sockets; tmp_bsock && tmp_bsock->next; tmp_bsock = tmp_bsock->next);
			tmp_bsock->next = bsock;
		}
	}
	if(!setup_bound_socket(bsock, port, queue_size)){
		MU_LOG_WARNING(logger, "setup_bound_socket: \"was unable to setup bsock\"\n");
		return NULL;
	}
	server->amount_of_sockets++;
	return bsock;
}

int NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	if(!server || !bsock || !bsock->sockfd) return 0;
	if(shutdown(bsock->sockfd, SHUT_RD) == -1){
		MU_LOG_BSOCK_ERR(shutdown, bsock);
		return 0;
	}
	NU_Client_Socket_t *client = NULL;
	for(client = server->clients; client; client = client->next){
		if(client->port == bsock->port){
			NU_Server_disconnect(server, client);
		}
	}
	server->amount_of_sockets--;
	if(shutdown(bsock->sockfd, SHUT_RDWR) == -1){
		MU_LOG_BSOCK_ERR(shutdown, bsock);
		return 0;
	}
	return 1;
}

NU_Client_Socket_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *bsock, unsigned int timeout){
	int is_reused = 1;
	NU_Client_Socket_t *client = reuse_existing_client(server->clients);
	if(!client) {
		client = calloc(1, sizeof(NU_Client_Socket_t));
		MU_ASSERT_RETURN(client, logger, NULL, "Was unable to allocate memory for client!\n");
		is_reused--;
	}
	if(!is_reused){
		client->bbuf = calloc(1, sizeof(NU_Bounded_Buffer_t));
		if(!server->clients) server->clients = client;
		else {
			NU_Client_Socket_t *tmp_client = NULL;
			for(tmp_client = server->clients; tmp_client && tmp_client->next; tmp_client = tmp_client->next);
			tmp_client->next = client;
		}
	}
	char *ip_addr;
	if(!(client->sockfd = timed_accept(bsock->sockfd, &ip_addr, timeout))){
		MU_LOG_INFO(logger, "accept: \"timed out\"\n");
		// Note that the client isn't freed and even if there is an error, it is still added to the list to be reused.
		return NULL;
	}
	strcpy(client->ip_address, ip_addr);
	/// Note to self: Optimize retrieval of IP Address, wasted extra, and very short lived allocation.
	free(ip_addr);
	client->port = bsock->port;
	server->amount_of_clients++;
	MU_LOG_SERVER(logger, "%s connected to port %d\n", client->ip_address, client->port);
	return client;
}

int NU_Server_send(NU_Server_t *server, NU_Client_Socket_t *client, const char *message, unsigned int timeout){
	if(!server || !client || !message) return 0;
	size_t buffer_size = strlen(message);
	size_t result = send_all(client->sockfd, message, timeout);
	server->data.bytes_sent += result;
	server->data.messages_sent++;
	if(result != buffer_size) MU_LOG_WARNING(logger, "Was unable to send all data to host!Total Sent: %d, Message Size: %d\n", result, buffer_size);
	return result;
}

const char *NU_Server_receive(NU_Server_t *server, NU_Client_Socket_t *client, size_t buffer_size, unsigned int timeout){
	if(!server || !client || !buffer_size) return NULL;
	resize_buffer(client->bbuf, buffer_size);
	size_t result = timed_receive(client->sockfd, client->bbuf, timeout);
	MU_LOG_VERBOSE(logger, "Total received: %d, Buffer Size: %d\n", result, buffer_size);
	if(!result) return NULL;
	client->bbuf->buffer[result] = '\0';
	return (const char *)client->bbuf->buffer;
}

int NU_Server_disconnect(NU_Server_t *server, NU_Client_Socket_t *client, const char *message){
	if(!server || !client) return 0;
	if(message){
		shutdown(client->sockfd, SHUT_RD);
		NU_Server_send(server, client, message, 0);
		shutdown(client->sockfd, SHUT_RDWR);
	} else shutdown(client->sockfd, SHUT_RDWR);
	MU_LOG_SERVER(logger, "%s disconnected from port %d\n", client->ip_address, client->port);
	client->sockfd = 0;
	return 1;
}