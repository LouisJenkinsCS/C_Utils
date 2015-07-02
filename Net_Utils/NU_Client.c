#include <NU_Client.h>

#define MU_LOG_CLIENT(message, ...) MU_LOG_CUSTOM(logger, "CLIENT", message, ##__VA_ARGS__)

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for NU_Client's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "NU_Client_Log.txt", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

static int get_server_socket(const char *host, unsigned int port, unsigned int is_udp, unsigned int timeout){
	struct addrinfo hints, *results, *current;
	fd_set connect_set;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	int retval, sockfd = 0, iteration = 0;
	char *port_str;
	asprintf(&port_str, "%u", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM;
	if(retval = getaddrinfo(host, port_str, &hints, &results)){
		MU_LOG_WARNING(logger, "get_server_socket->getaddrinfo: %s\n", gai_strerror(retval));
		free(port_str);
		return 0;
	}
	free(port_str);
	for(current = results; current; current = current->ai_next){
	    if((sockfd = TEMP_FAILURE_RETRY(socket(current->ai_family, current->ai_socktype, current->ai_protocol))) == -1) {
	      MU_LOG_VERBOSE(logger, "Skipped result with error \"%s\": Iteration #%d\n", strerror(errno), ++iteration);
	      continue;
	    }
	    MU_LOG_VERBOSE(logger, "Obtained a socket from a result: Iteration #%d\n", ++iteration);
	    FD_ZERO(&connect_set);
	    FD_SET(sockfd, &connect_set);
	    if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &connect_set, NULL, NULL, &tv))) <= 0){
	    	if(!retval) MU_LOG_VERBOSE(logger, "select: \"Timed out\"\n"); 
	    	else MU_LOG_VERBOSE(logger, "select: \"%s\"\n", strerror(errno));
	    	MU_LOG_VERBOSE(logger, "Iteration: #%d\n", ++iteration));
	    	continue;
	    }
	    if(TEMP_FAILURE_RETRY(connect(sockfd, current->ai_addr, current->ai_addrlen)) == -1){
	      close(sockfd);
	      MU_LOG_VERBOSE(logger, "connect->connect: \"%s\"; Iteration #%d\n", strerror(errno), ++iteration);
	      continue;
	    }
	    break;
	}
	return current ? sockfd : 0;
}

NU_Client_t *NU_Client_create(int flags){
	NU_Client_t *client = calloc(1, sizeof(NU_Client_t));
	if(!client) MU_LOG_ERROR(logger, "Was unable to allocate memory for client!\n");
	return client;
}

NU_Server_Socket_t *NU_Client_connect(MU_Client_t *client, const char *host, unsigned int port, unsigned int is_udp, unsigned int timeout){
	NU_Server_Socket_t *server = reuse_existing_server(client);
	int is_reused = 1;
	if(!server){ 
		server = calloc(1, sizeof(NU_Server_Socket_t));
		MU_ASSERT_RETURN(server, logger, NULL, "connect->calloc: \"Unable to allocate memory for server!\"\n");
		is_reused--;
	}
	if(!is_reused){
		if(!client->servers) client->servers = server;
		else{
			NU_Server_Socket_t *tmp_server = NULL;
			for(tmp_server = client->servers; tmp_server && tmp_server->next; tmp_server = tmp_server->next);
			tmp_server->next = server;
		}
	}
	if(!(server->sockfd = get_server_socket(host, port, is_udp))){
		MU_LOG_WARNING(logger, "connect->get_server_socket: \"Unable to connect to host\"\n");
		return NULL;
	}
	server->port = port;
	strcpy(server->ip_addr, host);
	client->amount_of_servers++;
	MU_LOG_CLIENT("Connected to %s on port %u\n", host, port);
	return 1;
}

int NU_Client_send(NU_Client_t *client, NU_Server_Socket_t *server, const char *message, unsigned int timeout){
	size_t buffer_size = strlen(message);
	size_t result = NUH_send_all(server->sockfd, message, timeout, logger);
	client->data->bytes_sent += result;
	client->data->messages_sent++;
	if(result != buffer_size) MU_LOG_WARNING(logger, "Was unable to send all data to server!Total Sent: %d, Message Size: %d\n", result, buffer_size);
	return result;
}

const char *NU_Client_recieve(NU_Client_t *client, size_t buffer_size, unsigned int timeout){
	NUH_resize_buffer(client->bbuf, buffer_size);
	size_t result = NUH_timed_received(client->sockfd, client->bbuf, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu\n", result, buffer_size);
	if(!result) return NULL;
	client->data.bytes_received += result;
	client->data.messages_received++;
	return (const char *)client->bbuf->buffer;
}

size_t NU_Client_send_file(NU_Client_t *client, NU_Server_Socket_t *server, FILE *file, size_t buffer_size, unsigned int timeout){
	if(!server || !client || !server->sockfd || !file || !buffer_size) return 0;
	NUH_resize_buffer(server->bbuf, buffer_size+1, logger);
	size_t retval, total_sent = 0;
	while((retval = fread(server->bbuf->buffer, 1, buffer_size, file)) == buffer_size){
		server->bbuf->buffer[retval] = '\0';
		if(NU_Client_send(client, server, server->bbuf->buffer, timeout) == 0){
			MU_LOG_WARNING(logger, "client_send_file->client_send: \"%s\"\n", "Was unable to send all of message to server!\n");
			return total_sent;
		}
		total_sent += retval;
	}
	if(!total_sent) MU_LOG_WARNING(logger, "No data was sent to server!\n");
	else client->data.messages_sent++;
	client->data.bytes_sent += (size_t) total_sent;
	return (size_t) total_sent;
}

size_t NU_Client_receive_to_file(NU_Client_t *client, NU_Server_Socket_t *server, FILE *file, size_t buffer_size, unsigned int is_binary, unsigned int timeout){
	if(!server || !client || !server->sockfd || !file || !buffer_size) return 0;
	NUH_resize_buffer(server->bbuf, buffer_size, logger);
	size_t result, total_received = 0;
	while((result = NUH_timed_receive(server->sockfd, server->bbuf, timeout, logger)) == buffer_size){
		if(is_binary) fwrite(server->bbuf->buffer, 1, server->bbuf->size, file);
		else fprintf(file, "%.*s", (int)buffer_size, server->bbuf->buffer);
		total_received += result;
	}
	if(result){
		if(is_binary) fwrite(server->bbuf->buffer, 1, result, file);
		else fprintf(file, "%.*s", (int)result, server->bbuf->buffer);
		total_received += result;
	}
	client->data.bytes_received += total_received;
	client->data.messages_received++;
	return total_received;
}

NU_Server_Socket_t **NU_Client_select_receive(NU_Client_t *client, NU_Server_Socket_t **servers, size_t *size, unsigned int timeout){
	if(!server || !clients || !size || !*size){
		*size = 0;
		return NULL;
	}
	fd_set receive_set;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	FD_ZERO(&receive_set);
	int max_fd = 0, retval;
	size_t i = 0, new_size = 0;
	for(;i < *size; i++){
		NU_Server_Socket_t *server = servers[i];
		if(!server || !server->sockfd) continue;
		FD_SET(server->sockfd, &receive_set);
		new_size++;
		if(server->sockfd > max_fd) max_fd = server->sockfd;
	}
	if(!new_size) {
		*size = 0;
		return NULL;
	}
	if((retval = TEMP_FAILURE_RETRY(select(max_fd + 1, &receive_set, NULL, NULL, &tv))) <= 0){
		if(!retval) MU_LOG_INFO(logger, "select_receive->select: \"timeout\"\n");
		else MU_LOG_WARNING(logger, "select_receive->select: \"%s\"\n", strerror(errno));
		*size = 0;
		return NULL;
	}
	NU_Server_Socket_t **ready_servers = malloc(sizeof(NU_Server_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(servers[i]->sockfd, &receive_set)) ready_servers[new_size++] = servers[i];
	*size = new_size;
	return ready_servers;
}

NU_Server_Socket_t **NU_Client_select_send(NU_Client_t *client, NU_Server_Socket_t **servers, size_t *size, unsigned int timeout){
	if(!server || !clients || !size || !*size){
		*size = 0;
		return NULL;
	}
	fd_set send_set;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	FD_ZERO(&send_set);
	int max_fd = 0, retval;
	size_t i = 0, new_size = 0;
	for(;i < *size; i++){
		NU_Client_Socket_t *server = servers[i];
		if(!server || !server->sockfd) continue;
		FD_SET(server->sockfd, &send_set);
		new_size++;
		if(server->sockfd > max_fd) max_fd = server->sockfd;
	}
	if(!new_size) {
		*size = 0;
		return NULL;
	}
	if((retval = TEMP_FAILURE_RETRY(select(max_fd + 1, NULL , &send_set, NULL, &tv))) <= 0){
		if(!retval) MU_LOG_VERBOSE(logger, "select_send->select: \"timeout\"\n");
		else MU_LOG_WARNING(logger, "select_send->select: \"%s\"\n", strerror(errno));
		*size = 0;
		return NULL;
	}
	NU_Client_Socket_t **ready_servers = malloc(sizeof(NU_Server_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(clients[i]->sockfd, &send_set)) ready_servers[new_size++] = servers[i];
	*size = new_size;
	return ready_servers;
}
}

char *NU_Client_about(MU_Client_t *client){
	char *message, *timestamp = Misc_Utils_get_timestamp();
	asprintf(&message, "Connected to: %s:%s\nMessages sent: %d; Total bytes: %d\nMessages received: %d; Total bytes: %d\n
		Connected from time %s to %s\n", client->host, client->port, client->data.messages_sent, client->data.bytes_sent,
		client->data.messages_received, client->data.bytes_received, client->timestamp, timestamp);
	free(timestamp);
	return message;
}

int NU_Client_shutdown(MU_Client_t *client){
	shutdown(client->sockfd, 2);
	MU_LOG_INFO(logger, "Client shutdown!\n");
	reset_client(client);
	return 1;
}

int NU_Client_destroy(MU_Client_t *client){
	MU_Client_shutdown(client);
	free(client->timestamp);
	free(client->bbuf);
	free(client);
	MU_LOG_INFO(logger, "Client destroyed!\n");
	return 1;
}