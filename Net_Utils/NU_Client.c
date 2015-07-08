#include <NU_Client.h>

#define MU_LOG_CLIENT(message, ...) MU_LOG_CUSTOM(logger, "CLIENT", message, ##__VA_ARGS__)

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for NU_Client's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "NU_Client.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

static int get_connection_socket(const char *host, unsigned int port, unsigned int timeout){
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
	hints.ai_socktype = SOCK_STREAM;
	if(retval = getaddrinfo(host, port_str, &hints, &results)){
		MU_LOG_WARNING(logger, "get_connection_socket->getaddrinfo: \"%s\"\n", gai_strerror(retval));
		free(port_str);
		return -1;
	}
	free(port_str);
	// Loop through all potential results to find a valid connection.
	for(current = results; current; current = current->ai_next){
	    if((sockfd = TEMP_FAILURE_RETRY(socket(current->ai_family, current->ai_socktype, current->ai_protocol))) == -1){
	      MU_LOG_VERBOSE(logger, "get_connection_socket->socket: \"%s\": Iteration #%d\n", strerror(errno), ++iteration);
	      continue;
	    }
	    MU_LOG_VERBOSE(logger, "get_connection_socket: \"Received a socket!\": Iteration #%d\n", ++iteration);
	    FD_ZERO(&connect_set);
	    FD_SET(sockfd, &connect_set);
	    if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &connect_set, NULL, NULL, &tv))) <= 0){
	    	if(!retval) MU_LOG_VERBOSE(logger, "get_connection_socket->select: \"Timed out!\": Iteration: #%d\n", ++iteration);
	    	else MU_LOG_VERBOSE(logger, "get_connection_socket->select: \"%s\": Iteration: #%d\n", strerror(errno), ++iteration);
	    	TEMP_FAILURE_RETRY(close(sockfd));
	    	continue;
	    }
	    if(TEMP_FAILURE_RETRY(connect(sockfd, current->ai_addr, current->ai_addrlen)) == -1){
	      MU_LOG_VERBOSE(logger, "get_connection_socket->connect: \"%s\"; Iteration #%d\n", strerror(errno), ++iteration);
	      TEMP_FAILURE_RETRY(close(sockfd));
	      continue;
	    }
	    break;
	} 	
	/* 
		Note: If current is not NULL, then it succeeded in finding a socket, hence it is safe to return it.
		0 as a file descriptor represents stdin, which is impossible to have as an output fd.
	*/
	return current ? sockfd : -1;
}

NU_Client_t *NU_Client_create(size_t initial_size, unsigned char init_locks){
	NU_Client_t *client = calloc(1, sizeof(NU_Client_t));
	MU_ASSERT_RETURN(client, logger, NULL, "NU_Client_create->calloc: \"%s\"\n", strerror(errno));
	if(init_locks){
		client->lock = malloc(sizeof(pthread_rwlock_t));
		if(!client->lock){
			free(client);
			MU_ASSERT_RETURN(client->lock, logger, NULL, "NU_Client_create->malloc: \"%s\"\n", strerror(errno));
		}
		int failure = pthread_rwlock_init(clinet->lock, NULL);
		if(failure){
			MU_LOG_ERROR(logger, "NU_Client_create->pthread_rwlock_init: \"%s\"\n", strerror(failure));
			free(client->lock);
			free(client);
			return NULL;
		}
	}
	client->amount_of_connections = initial_size;
	client->connections = malloc(sizeof(NU_Connection_t *) * (initial_size ? initial_size : 1));
	size_t i = 0;
	for(;i < initial_size; i++){
		NU_Connection_t *conn = NU_Connection_create(NU_CLIENT, init_locks, logger);
		/// If the connection failed to be created, then we must deallocate all memory we attempted to allocate before.
		if(!conn){
			size_t j = 0;
			for(;j < i; j++){
				NU_Connection_destroy(client->connections[j], logger);
			}
			free(client->connections);
			if(init_locks){
				int failure = pthread_rwlock_destroy(conn->lock);
				if(failure){
					MU_LOG_ERROR(logger, "NU_Client_create->pthread_rwlock_destroy: \"%s\"\n", strerror(failure));
				}
				free(conn->lock);
			}
			MU_ASSERT_RETURN(conn, logger, NULL, "NU_Client_create->NU_Connection_create: \"Was unable to create connection #%d!\"\n", ++i);
		}
		client->connections[i] = conn;
	}
	return client;
}

NU_Connection_t *NU_Client_connect(NU_Client_t *client, unsigned int init_locks, const char *ip_addr, unsigned int port, unsigned int timeout){
	if(!client || !ip_addr || !port){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Client: %s;IP Address: %s;Port: %s\"\n",
			client ? "OK!" : "NULL", ip_addr ? "OK!" : "NULL", port ? "OK!" : "NULL");
		return NULL;
	}
	int sockfd = get_connection_socket(ip_addr, port, timeout);
	if(sockfd == -1){
		MU_LOG_WARNING(logger, "NU_Client_connect->get_server_socket: \"Was unable to form a connection!\"\n");
		NU_rwlock_unlock(client->lock, logger);
		return NULL;
	}
	NU_rwlock_wrlock(client->lock, logger);
	NU_Connection_t *conn = NU_reuse_connection(client->connections, logger);
	if(!conn){
		conn = NU_Connection_create(NU_CLIENT,init_locks, logger);
		if(!conn){
			NU_rwlock_unlock(client->lock, logger);
			MU_ASSERT_RETURN(conn, logger, NULL, "NU_Client_connect->NU_Connection_create: \"Was unable to create connection!\"\n");
		}
		NU_Connection_t **tmp_connections = realloc(client->connections, sizeof(NU_Connection_t *) * client->amount_of_connections + 1);
		if(!tmp_connections){
			NU_rwlock_unlock(client->lock, logger);
			MU_ASSERT_RETURN(tmp_connections, logger, NULL, "NU_Client_connect->realloc: \"%s\"\n", strerror(errno));
		}
		client->connections = tmp_connections;
		client->connections[client->amount_of_connections] = conn;
		client->amount_of_connections++;
	}
	int successful = NU_Connection_init(conn, sockfd, port, ip_addr, logger);
	NU_rwlock_unlock(client->lock, logger);
	if(!successful){
		MU_LOG_WARNING(logger, "NU_Client_connect->NU_Connection_init: \"Was unable to iniitalize client!\"\n");
		return NULL;
	}
	MU_LOG_INFO(logger, "Connected to %s on port %u\n", ip_addr, port);
	return conn;
}

size_t NU_Client_send(NU_Client_t *client, NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout){
	if(!client || !conn || conn->type != NU_CLIENT){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Client: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	NU_rwlock_rdlock(client->lock);
	size_t result = NU_Connection_send(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total Sent: %zu, Buffer Size: %zu\n", result, buf_size);
	client->data.bytes_sent += result;
	client->data.messages_sent++;
	if(result != buf_size){
		MU_LOG_WARNING(logger, "NU_Client_send->NU_Connection_send: \"Was unable to send %zu bytes to %s!\"\n", buf_size - result, NU_Connection_get_ip_addr(conn));
	}
	NU_rwlock_unlock(client->lock);
	return result;
}

size_t NU_Client_receive(NU_Client_t *client, NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout){
	if(!client || !conn || conn->type != NU_Client){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Client: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	NU_rwlock_rdlock(client->lock);
	size_t result = NU_Connection_receive(conn, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu\n", result, buf_size);
	if(!result){
		MU_LOG_WARNING(logger, "NU_Client_receive->NU_Connection_receive: \"Was unable to receive from %s!\"\n", NU_Connection_get_ip_addr(conn));
		return 0;
	}
	client->data.bytes_received += result;
	client->data.messages_received++;
	NU_rwlock_unlock(client->lock);
	return result;
}

size_t NU_Client_send_file(NU_Client_t *client, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	if(!client || !conn || !file || conn->type != NU_CLIENT){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Client: %s;File: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", file ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	// Obtain the file size to aid in determining whether or not the send was successful or not.
	struct stat file_stats;
	int file_fd = fileno(file);
	if(file_fd == -1){
		MU_LOG_WARNING(logger, "NU_Client_send_file->fileno: \"%s\"\n", strerror(errno));
	}
	size_t file_size = 0;
	if(fstat(file_fd, &file_stats) == -1){
		MU_LOG_WARNING(logger, "NU_Client_send_file->fstat: \"%s\"\n", strerror(errno));
	}
	else {
		file_size = file_stats.st_size;
	}
	NU_rwlock_rdlock(client->lock);
	size_t total_sent = NU_Connection_send_file(conn, file, buf_size, timeout, logger);
	// Note that if the file size is zero, meaning that there was an error with fstat, it will skip this check.
	if(!total_sent) MU_LOG_WARNING(logger, "NU_Client_send_file->NU_Connection_send_file: \"No data was sent to %s\"\n", NU_Connection_get_ip_addr(conn));
	else if(file_size && file_size != total_sent){ 
		MU_LOG_WARNING(logger, "NU_Client_send_file->NU_Connection_send_file: \"File Size is %zu, but only sent %zu to %s\"\n",
			file_size, total_sent, NU_Connection_get_ip_addr(conn));
	}
	else client->data.messages_sent++;
	client->data.bytes_sent += total_sent;
	NU_rwlock_unlock(client->lock);
	MU_LOG_VERBOSE(logger, "Sent file of total size %zu to %s!\n", total_sent, NU_Connection_get_ip_addr(conn));
	return total_sent;
}

size_t NU_Client_receive_to_file(NU_Client_t *client, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	if(!client || !conn || conn->type != NU_CLIENT){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Client: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	NU_rwlock_rdlock(clinet->lock);
	size_t total_received = NU_Connection_receive_to_file(conn, file, buf_size, timeout, logger);
	if(!total_received) MU_LOG_WARNING(logger, "NU_Client_receive_to_file->NU_Connection_receive_to_file: \"Was unable to receive file from %s\"\n", NU_Connection_get_ip_addr(conn));
	else client->data.messages_received++;
	client->data.bytes_received += total_received;
	NU_rwlock_unlock(client->lock);
	MU_LOG_VERBOSE(logger, "Received file of total size %zu from %s!\n", total_received, conn->ip_addr);
	return total_received;
}

NU_Connection_t **NU_Client_select_receive(NU_Client_t *client, NU_Connection_t **connections, size_t *size, unsigned int timeout){
	if(!servers || !client || !size || !*size){
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
		NU_Connection_t *conn = servers[i];
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
	NU_Connection_t **ready_servers = malloc(sizeof(NU_Server_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(servers[i]->sockfd, &receive_set)) ready_servers[new_size++] = servers[i];
	*size = new_size;
	return ready_servers;
}

NU_Connection_t **NU_Client_select_send(NU_Client_t *client, NU_Connection_t **connections, size_t *size, unsigned int timeout){
	if(!servers || !client || !size || !*size){
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
		NU_Connection_t *conn = servers[i];
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
	NU_Connection_t **ready_servers = malloc(sizeof(NU_Server_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(servers[i]->sockfd, &send_set)) ready_servers[new_size++] = servers[i];
	*size = new_size;
	return ready_servers;
}

char *NU_Client_about(NU_Client_t *client){
	return NULL;
}

int NU_Client_shutdown(NU_Client_t *client, const char *msg){
	return 0;
}

int NU_Client_destroy(NU_Client_t *client, const char *msg){
	return 0;
}