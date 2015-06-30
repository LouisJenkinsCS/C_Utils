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

NU_Client_t *NU_Client_create(int flags){
	MU_Client_t *client = calloc(1, sizeof(NU_Client_t));
	if(!client) MU_LOG_ERROR(logger, "Was unable to allocate memory for client!\n");
	return client;
}

NU_Server_Socket_t *NU_Client_connect(MU_Client_t *client, const char *host, unsigned int port, int flags){
	NU_Server_Socket_t *server = reuse_existing_server(client);
	int is_reused = 1;
	if(!server){ 
		server = calloc(1, sizeof(NU_Server_Socket_t));
		MU_ASSERT_RETURN(server, logger, NULL, "Was unable to allocate memory for server socket!\n");
		is_reused--;
	}
	if(!is_reused){
		if(!client->servers) client->servers = server;
		else{
			NU_Server_Socket_t *tmp_server = NULL;
			for(tmp_server = client->servers; tmp_server; tmp_server = tmp_server->next);
			tmp_server->next = server;
		}
	}
	// TODO: Continue here
	struct addrinfo *results;
	int retval;
	memset(&socket_options, 0, sizeof(socket_options));
	socket_options.ai_family = AF_INET;
	socket_options.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(host, port, &socket_options, &results) == -1){
		MU_LOG_WARNING(logger, "Unable to get addrinfo: %s\n", gai_strerror(errno));
		return 0;
	}
	if((client->sockfd = get_socket(results)) == -1){
		MU_LOG_WARNING(logger, "Was unable to find a valid address!\n");
		freeaddrinfo(results);
		return 0;
	}
	freeaddrinfo(results);
	return 1;
}

int NU_Client_send(NU_Client_t *client, char *message, unsigned int timeout){
	size_t buffer_size = strlen(message);
	size_t result = send_all(client->sockfd, message, timeout);
	client->data->bytes_sent += result;
	client->data->messages_sent++;
	if(result != buffer_size) MU_LOG_WARNING(logger, "Was unable to send all data to host!Total Sent: %d, Message Size: %d\n", result, buffer_size);
	return result;
}

const char *NU_Client_recieve(NU_Client_t *client, size_t buffer_size, unsigned int timeout){
	resize_buffer(client->bbuf, buffer_size);
	size_t result = receive_all(client->sockfd, client->bbuf, timeout);
	client->data.bytes_received += result;
	client->data.messages_received++;
	client->bbuf->buffer[result] = '\0';
	if(result != buffer_size) MU_LOG_WARNING(logger, "Was unable to recieve enough data to fill the buffer! Total received: %d, Buffer Size: %d\n", result, buffer_size);
	return (const char *)client->bbuf->buffer;
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