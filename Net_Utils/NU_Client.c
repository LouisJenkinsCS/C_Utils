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

NU_Client_t *NU_Client_create(size_t connection_pool_size, unsigned char init_locks){
	NU_Client_t *client = calloc(1, sizeof(NU_Client_t));
	if(!client){
		MU_LOG_ASSERT(logger, "NU_Client_create->calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	size_t connections_allocated = 0;
	client->data = NU_Atomic_Data_create();
	if(!client->data){
		MU_LOG_ASSERT(logger, "NU_Client_create->NU_Collective_Data_create: \"Was unable to allocate atomic data\"\n");
		goto error;
	}
	if(init_locks){
		client->lock = malloc(sizeof(pthread_rwlock_t));
		if(!client->lock){
			MU_LOG_ASSERT(logger, "NU_Client_create->malloc: \"%s\"\n", strerror(errno));
			goto error;
		}
		int failure = pthread_rwlock_init(clinet->lock, NULL);
		if(failure){
			MU_LOG_ERROR(logger, "NU_Client_create->pthread_rwlock_init: \"%s\"\n", strerror(failure));
			goto error;
		}
	}
	client->amount_of_connections = connection_pool_size ? connection_pool_size : 1;
	client->connections = calloc(client->amount_of_connections, sizeof(NU_Connection_t *));
	if(!client->connections){
		MU_LOG_ASSERT(logger, "NU_Client_create->calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	size_t i = 0;
	for(;i < client->connections; i++){
		NU_Connection_t *conn = NU_Connection_create(NU_CLIENT, init_locks, logger);
		if(!conn){
			MU_LOG_ASSERT(logger, "NU_Client_create->NU_Connection_create: \"Was unable to create connection #%d!\"\n", ++i);
			goto error;
		}
		client->connections[i] = conn;
	}
	return client;
	/// Deallocate all memory allocated if anything fails!
	error:
		if(client->connections){
			size_t i = 0;
			for(;i < connections_allocated; i++){
				int is_destroyed = NU_Connection_destroy(client->connections[i], logger);
				if(!is_destroyed){
					MU_LOG_ERROR(logger, "NU_Client_create->NU_Connection_destroy: \"Was unable to destroy a connection\"\n");
				}
			}
			free(client->connections);
		}
		if(init_locks){
			MU_Cond_rwlock_destroy(server->lock);
		}
		if(client->data){
			free(client->data);
		}
		if(client){
			free(client);
		}
		return NULL;

}

NU_Connection_t *NU_Client_connect(NU_Client_t *client, unsigned int init_locks, const char *ip_addr, unsigned int port, unsigned int timeout){
	if(!client || !ip_addr || !port){
		MU_LOG_ERROR(logger, "NU_Client_connect: Invalid Arguments=> \"Client: %s;IP Address: %s;Port: %s\"\n",
			client ? "OK!" : "NULL", ip_addr ? "OK!" : "NULL", port ? "OK!" : "NULL");
		return NULL;
	}
	int sockfd = get_connection_socket(ip_addr, port, timeout);
	if(sockfd == -1){
		MU_LOG_WARNING(logger, "NU_Client_connect->get_server_socket: \"Was unable to form a connection!\"\n");
		MU_Cond_rwlock_unlock(client->lock, logger);
		return NULL;
	}
	MU_Cond_rwlock_wrlock(client->lock, logger);
	NU_Connection_t *conn = NU_reuse_connection(client->connections, logger);
	if(!conn){
		conn = NU_Connection_create(NU_CLIENT,init_locks, logger);
		if(!conn){
			MU_Cond_rwlock_unlock(client->lock, logger);
			MU_ASSERT_RETURN(conn, logger, NULL, "NU_Client_connect->NU_Connection_create: \"Was unable to create connection!\"\n");
		}
		NU_Connection_t **tmp_connections = realloc(client->connections, sizeof(NU_Connection_t *) * client->amount_of_connections + 1);
		if(!tmp_connections){
			MU_Cond_rwlock_unlock(client->lock, logger);
			MU_ASSERT_RETURN(tmp_connections, logger, NULL, "NU_Client_connect->realloc: \"%s\"\n", strerror(errno));
		}
		client->connections = tmp_connections;
		client->connections[client->amount_of_connections++] = conn;
	}
	int successful = NU_Connection_init(conn, sockfd, port, ip_addr, logger);
	MU_Cond_rwlock_unlock(client->lock, logger);
	if(!successful){
		MU_LOG_WARNING(logger, "NU_Client_connect->NU_Connection_init: \"Was unable to iniitalize client!\"\n");
		return NULL;
	}
	MU_LOG_INFO(logger, "Connected to %s on port %u\n", ip_addr, port);
	return conn;
}

size_t NU_Client_send(NU_Client_t *client, NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout){
	if(!client || !conn || conn->type != NU_CLIENT){
		MU_LOG_ERROR(logger, "NU_Client_send: Invalid Arguments=> \"Client: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(client->lock, logger);
	size_t result = NU_Connection_send(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total Sent: %zu, Buffer Size: %zu\n", result, buf_size);
	NU_Atomic_Data_increment_sent(client->data, result);
	if(result != buf_size){
		MU_LOG_WARNING(logger, "NU_Client_send->NU_Connection_send: \"Was unable to send %zu bytes to %s!\"\n", buf_size - result, NU_Connection_get_ip_addr(conn));
	}
	MU_Cond_rwlock_unlock(client->lock, logger);
	return result;
}

size_t NU_Client_receive(NU_Client_t *client, NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout){
	if(!client || !conn || conn->type != NU_Client){
		MU_LOG_ERROR(logger, "NU_Client_receive: Invalid Arguments=> \"Client: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(client->lock, logger);
	size_t result = NU_Connection_receive(conn, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu\n", result, buf_size);
	if(!result){
		MU_LOG_WARNING(logger, "NU_Client_receive->NU_Connection_receive: \"Was unable to receive from %s!\"\n", NU_Connection_get_ip_addr(conn));
		return 0;
	}
	NU_Atomic_Data_increment_received(client->data, result);
	MU_Cond_rwlock_unlock(client->lock, logger);
	return result;
}

size_t NU_Client_send_file(NU_Client_t *client, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	if(!client || !conn || !file || conn->type != NU_CLIENT){
		MU_LOG_ERROR(logger, "NU_Client_send_file: Invalid Arguments=> \"Client: %s;File: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", file ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	// Obtain the file size to aid in determining whether or not the send was successful or not.
	struct stat file_stats
;	int file_fd = fileno(file);
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
	MU_Cond_rwlock_rdlock(client->lock, logger);
	size_t total_sent = NU_Connection_send_file(conn, file, buf_size, timeout, logger);
	// Note that if the file size is zero, meaning that there was an error with fstat, it will skip this check.
	if(!total_sent){
		MU_LOG_WARNING(logger, "NU_Client_send_file->NU_Connection_send_file: \"No data was sent to %s\"\n", NU_Connection_get_ip_addr(conn));
		MU_Cond_rwlock_unlock(client->lock, logger);
		return 0;
	}
	else if(file_size && file_size != total_sent){ 
		MU_LOG_WARNING(logger, "NU_Client_send_file->NU_Connection_send_file: \"File Size is %zu, but only sent %zu to %s\"\n",
			file_size, total_sent, NU_Connection_get_ip_addr(conn));
	}
	NU_Atomic_Data_increment_sent(client->data, total_sent);
	MU_Cond_rwlock_unlock(client->lock, logger);
	MU_LOG_VERBOSE(logger, "Sent file of total size %zu to %s!\n", total_sent, NU_Connection_get_ip_addr(conn));
	return total_sent;
}

size_t NU_Client_receive_to_file(NU_Client_t *client, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	if(!client || !conn || conn->type != NU_CLIENT){
		MU_LOG_ERROR(logger, "NU_Client_receive_to_file: Invalid Arguments=> \"Client: %s;Connection: %s;Connection-Type: %s\"\n",
			client ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(clinet->lock, logger);
	size_t total_received = NU_Connection_receive_to_file(conn, file, buf_size, timeout, logger);
	if(!total_received) MU_LOG_WARNING(logger, "NU_Client_receive_to_file->NU_Connection_receive_to_file: \"Was unable to receive file from %s\"\n", NU_Connection_get_ip_addr(conn));
	NU_Atomic_Data_increment_received(client->data, result);
	MU_Cond_rwlock_unlock(client->lock, logger);
	MU_LOG_VERBOSE(logger, "Received file of total size %zu from %s!\n", total_received, conn->ip_addr);
	return total_received;
}

NU_Connection_t **NU_Client_select_receive(NU_Client_t *client, NU_Connection_t **connections, size_t *size, unsigned int timeout){
	if(!client){
		MU_LOG_ERROR(logger, "NU_Client_select_receive: Invalid Argument=> \"Client: %s\"\n", client ? "OK!" : "NULL");
		*size = 0;
		return NULL;
	}
	MU_Cond_rwlock_rdlock(client->lock, logger);
	NU_Connection_t **ready_connections = NU_select_receive_connections(connections, size, timeout, logger);
	MU_Cond_rwlock_unlock(client->lock, logger);
	return ready_connections;
}

NU_Connection_t **NU_Client_select_send(NU_Client_t *client, NU_Connection_t **connections, size_t *size, unsigned int timeout){
	if(!client){
		MU_LOG_ERROR(logger, "NU_Client_select_send: Invalid Argument=> \"Client: %s\"\n", client ? "OK!" : "NULL");
		*size = 0;
		return NULL;
	}
	MU_Cond_rwlock_rdlock(client->lock, logger);
	NU_Connection_t **ready_connections = NU_select_send_connections(connections, size, timeout, logger);
	MU_Cond_rwlock_unlock(client->lock, logger);
	return ready_connections;
}

char *NU_Client_about(NU_Client_t *client){
	if(!client){
		MU_LOG_ERROR(logger, "NU_Client_about: Invalid Argument=> \"Client: %s\"\n", client ? "OK!" : "NULL");
		return NULL;
	}
	MU_Cond_rwlock_rdlock(client->lock, logger);
	char *client_str = "{ ", *old_client_str;
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		old_client_str = client_str;
		asprintf(&client_str, "%s%s%s", client_str, NU_Connection_to_string(client->connections[i]), i < client->amount_of_connections - 1 ? "," : "");
		// Since client_str is originally a string literal, freeing it would cause a segmentation fault, so we only skip the first one.
		if(i > 0){
			free(old_client_str);
		}
	}
	old_client_str = client_str;
	asprintf(&client_str, "%s }", client_str);
	free(old_client_str);
	MU_Cond_rwlock_unlock(client->lock, logger);
	return client_str;
}

int NU_Client_log(NU_Server_t *server, const char *message, ...){
	if(!client || !message){
		MU_LOG_ERROR(logger, "NU_Client_log: Invalid Arguments=> \"Client: %s;Message: %s\"\n", client ? "OK!" : "NULL", message ? "OK!" : "NULL");
		return 0;
	}
	va_list args;
	va_start(args, message);
	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		MU_LOG_WARNING(logger, "NU_Client_log->vsnprintf: \"%s\"\n", strerror(errno));
		return 0;
	}
	MU_LOG_CLIENT("%s", buffer);
	return 1;
}

int NU_Client_disconnect(NU_Client_t *client, NU_Connection_t *connection){
	if(!client){
		MU_LOG_ERROR(logger, "NU_Client_disconnect: Invalid Argument=> \"Client: %s\"\n", client ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_wrlock(client->lock, logger);
	int successful = NU_Connection_disconnect(connection, logger);
	if(!successful){
		MU_LOG_ERROR(logger, "NU_Client_disconnect->NU_Connection_disconnect: \"Was unable to fully disconnect a connection!\"\n");
	}
	MU_Cond_rwlock_unlock(client->lock, logger);
}

int NU_Client_shutdown(NU_Client_t *client){
	if(!client){
		MU_LOG_ERROR(logger, "NU_Client_shutdown: Invalid Arguments=> \"Client: %s\"\n", client ? "OK!" : "NULL");
		return 0;
	}
	int fully_shutdown = 1;
	MU_Cond_rwlock_wrlock(client->lock, logger);
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		int successful = NU_Connection_disconnect(client->connections[i], logger);
		if(!successful){
			fully_shutdown = 0;
			MU_LOG_ERROR(logger, "NU_Client_shutdown->NU_Connection_shutdown: \"Was unable to fully shutdown a connection!\"\n");
		}
	}
	MU_Cond_rwlock_unlock(client->lock, logger);
	return 1;
}

int NU_Client_destroy(NU_Client_t *client){
	if(!client){
		MU_LOG_ERROR(logger, "NU_Client_destroy: Invalid Arguments=> \"Client: %s\"\n", client ? "OK!" : "NULL");
		return 0;
	}
	int fully_destroyed = 1;
	MU_Cond_rwlock_wrlock(client->lock, logger);
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		int successful = NU_Connection_destroy(client->connections[i], logger);
		if(!successful){
			fully_destroyed = 0;
			MU_LOG_ERROR(logger, "NU_Client_destroy->NU_Connection_destroy: \"Was unable to fully destroy a connection!\"\n");
		}
	}
	MU_Cond_rwlock_unlock(client->lock, logger);
	if(conn->lock){
		int successful = pthread_rwlock_destroy(conn->lock);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Client_destroy->pthread_rwlock_destroy: \"%s\"\n", strerror(successful));
			fully_destroyed = 0;
		}
		else{
			// Note that the lock is only freed if it successfull deallocates it. If EBUSY ends up being returned, then it's only a memory leak and not
			// undefined behavior.
			free(conn->lock);
		}
	}
	free(client->data);
	return fully_destroyed;
}