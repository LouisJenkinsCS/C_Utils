#include <NU_Client.h>

#define MU_LOG_CLIENT(message, ...) MU_LOG_CUSTOM(logger, "CLIENT", message, ##__VA_ARGS__)

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("NU_Client.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
	free(logger);
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
	if((retval = getaddrinfo(host, port_str, &hints, &results))){
		MU_LOG_WARNING(logger, "getaddrinfo: \"%s\"\n", gai_strerror(retval));
		free(port_str);
		return -1;
	}
	free(port_str);
	// Loop through all potential results to find a valid connection.
	for(current = results; current; current = current->ai_next){
		MU_TEMP_FAILURE_RETRY(sockfd, socket(current->ai_family, current->ai_socktype, current->ai_protocol));
	    if(sockfd == -1){
	      MU_LOG_VERBOSE(logger, "socket: \"%s\": Iteration #%d\n", strerror(errno), ++iteration);
	      continue;
	    }
	    MU_LOG_VERBOSE(logger, "get_connection_socket: \"Received a socket!\": Iteration #%d\n", ++iteration);
	    FD_ZERO(&connect_set);
	    FD_SET(sockfd, &connect_set);
	    MU_TEMP_FAILURE_RETRY(retval, select(sockfd + 1, &connect_set, NULL, NULL, &tv));
	    if(retval <= 0){
	    	if(!retval) MU_LOG_VERBOSE(logger, "select: \"Timed out!\": Iteration: #%d\n", ++iteration);
	    	else MU_LOG_VERBOSE(logger, "select: \"%s\": Iteration: #%d\n", strerror(errno), ++iteration);
	    	MU_TEMP_FAILURE_RETRY(retval, close(sockfd));
	    	continue;
	    }
	    MU_TEMP_FAILURE_RETRY(retval, connect(sockfd, current->ai_addr, current->ai_addrlen));
	    if(retval == -1){
	      MU_LOG_VERBOSE(logger, "connect: \"%s\"; Iteration #%d\n", strerror(errno), ++iteration);
	      MU_TEMP_FAILURE_RETRY(retval, close(sockfd));
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

NU_Client_t *NU_Client_create(size_t connection_pool_size, bool init_locks){
	NU_Client_t *client = calloc(1, sizeof(NU_Client_t));
	if(!client){
		MU_LOG_ASSERT(logger, "calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	size_t connections_allocated = 0;
	if(init_locks){
		client->lock = malloc(sizeof(pthread_rwlock_t));
		if(!client->lock){
			MU_LOG_ASSERT(logger, "malloc: \"%s\"\n", strerror(errno));
			goto error;
		}
		int failure = pthread_rwlock_init(client->lock, NULL);
		if(failure){
			MU_LOG_ERROR(logger, "pthread_rwlock_init: \"%s\"\n", strerror(failure));
			goto error;
		}
		client->is_threaded = 1;
	}
	client->amount_of_connections = connection_pool_size ? connection_pool_size : 1;
	client->connections = calloc(client->amount_of_connections, sizeof(NU_Connection_t *));
	if(!client->connections){
		MU_LOG_ASSERT(logger, "calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		NU_Connection_t *conn = NU_Connection_create(init_locks, logger);
		if(!conn){
			MU_LOG_ASSERT(logger, "NU_Connection_create: \"Was unable to create connection #%d!\"\n", ++i);
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
					MU_LOG_ERROR(logger, "NU_Connection_destroy: \"Was unable to destroy a connection\"\n");
				}
			}
			free(client->connections);
		}
		MU_COND_RWLOCK_DESTROY(client->lock, logger);
		if(client){
			free(client);
		}
		return NULL;

}

NU_Connection_t *NU_Client_connect(NU_Client_t *client, const char *ip_addr, unsigned int port, unsigned int timeout){
	MU_ARG_CHECK(logger, NULL, client, ip_addr, port > 0);
	int sockfd = get_connection_socket(ip_addr, port, timeout);
	if(sockfd == -1){
		MU_LOG_WARNING(logger, "get_server_socket: \"Was unable to form a connection!\"\n");
		MU_COND_RWLOCK_UNLOCK(client->lock, logger);
		return NULL;
	}
	MU_COND_RWLOCK_RDLOCK(client->lock, logger);
	NU_Connection_t *conn = NU_Connection_reuse(client->connections, client->amount_of_connections, sockfd, port, ip_addr, logger);
	if(conn){
		MU_LOG_INFO(logger, "Connected to %s on port %u\n", ip_addr, port);
		MU_COND_RWLOCK_UNLOCK(client->lock, logger);
		return conn;
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	MU_COND_RWLOCK_WRLOCK(client->lock, logger);
	conn = NU_Connection_create(client->is_threaded, logger);
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_create: \"Was unable to create connection!\"\n");
		MU_COND_RWLOCK_UNLOCK(client->lock, logger);
		return NULL;
	}
	NU_Connection_t **tmp_connections = realloc(client->connections, sizeof(NU_Connection_t *) * client->amount_of_connections + 1);
	if(!tmp_connections){
		MU_COND_RWLOCK_UNLOCK(client->lock, logger);
		MU_LOG_ASSERT(logger, "realloc: \"%s\"\n", strerror(errno));
		return NULL;
	}
	client->connections = tmp_connections;
	client->connections[client->amount_of_connections++] = conn;
	int successful = NU_Connection_init(conn, sockfd, port, ip_addr, logger);
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	if(!successful){
		MU_LOG_WARNING(logger, "NU_Connection_init: \"Was unable to iniitalize client!\"\n");
		return NULL;
	}
	MU_LOG_INFO(logger, "Connected to %s on port %u\n", ip_addr, port);
	return conn;
}

size_t NU_Client_send(NU_Client_t *client, NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, client, conn, buffer, buf_size > 0);
	MU_COND_RWLOCK_RDLOCK(client->lock, logger);
	size_t result = NU_Connection_send(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total Sent: %zu, Buffer Size: %zu\n", result, buf_size);
	if(result != buf_size){
		MU_LOG_WARNING(logger, "NU_Connection_send: \"Was unable to send %zu bytes to %s!\"\n", buf_size - result, NU_Connection_get_ip_addr(conn, logger));
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	return result;
}

size_t NU_Client_receive(NU_Client_t *client, NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, conn, buffer, buf_size > 0);
	MU_COND_RWLOCK_RDLOCK(client->lock, logger);
	size_t result = NU_Connection_receive(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu\n", result, buf_size);
	if(!result){
		MU_LOG_WARNING(logger, "NU_Connection_receive: \"Was unable to receive from %s!\"\n", NU_Connection_get_ip_addr(conn, logger));
		return 0;
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	return result;
}

size_t NU_Client_send_file(NU_Client_t *client, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, client, conn, file, buf_size > 0);
	// Obtain the file size to aid in determining whether or not the send was successful or not.
	struct stat file_stats
;	int file_fd = fileno(file);
	if(file_fd == -1){
		MU_LOG_WARNING(logger, "fileno: \"%s\"\n", strerror(errno));
	}
	size_t file_size = 0;
	if(fstat(file_fd, &file_stats) == -1){
		MU_LOG_WARNING(logger, "fstat: \"%s\"\n", strerror(errno));
	}
	else {
		file_size = file_stats.st_size;
	}
	MU_COND_RWLOCK_RDLOCK(client->lock, logger);
	size_t total_sent = NU_Connection_send_file(conn, file, buf_size, timeout, logger);
	// Note that if the file size is zero, meaning that there was an error with fstat, it will skip this check.
	if(!total_sent){
		MU_LOG_WARNING(logger, "NU_Connection_send_file: \"No data was sent to %s\"\n", NU_Connection_get_ip_addr(conn, logger));
		MU_COND_RWLOCK_UNLOCK(client->lock, logger);
		return 0;
	}
	else if(file_size && file_size != total_sent){ 
		MU_LOG_WARNING(logger, "NU_Connection_send_file: \"File Size is %zu, but only sent %zu to %s\"\n",
			file_size, total_sent, NU_Connection_get_ip_addr(conn, logger));
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	MU_LOG_VERBOSE(logger, "Sent file of total size %zu to %s!\n", total_sent, NU_Connection_get_ip_addr(conn, logger));
	return total_sent;
}

size_t NU_Client_receive_file(NU_Client_t *client, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, client, conn, file, buf_size > 0);
	MU_COND_RWLOCK_RDLOCK(client->lock, logger);
	size_t total_received = NU_Connection_receive_file(conn, file, buf_size, timeout, logger);
	if(!total_received) MU_LOG_WARNING(logger, "NU_Connection_receive_to_file: \"Was unable to receive file from %s\"\n", NU_Connection_get_ip_addr(conn, logger));
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	MU_LOG_VERBOSE(logger, "Received file of total size %zu from %s!\n", total_received, conn->ip_addr);
	return total_received;
}

bool NU_Client_log(NU_Client_t *client, const char *message, ...){
	MU_ARG_CHECK(logger, false, client, message);
	va_list args;
	va_start(args, message);
	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		MU_LOG_WARNING(logger, "vsnprintf: \"%s\"\n", strerror(errno));
		return false;
	}
	MU_LOG_CLIENT("%s", buffer);
	return true;
}

bool NU_Client_disconnect(NU_Client_t *client, NU_Connection_t *conn){
	MU_ARG_CHECK(logger, false, client, conn);
	MU_COND_RWLOCK_WRLOCK(client->lock, logger);
	int successful = NU_Connection_disconnect(conn, logger);
	if(!successful){
		MU_LOG_ERROR(logger, "NU_Connection_disconnect: \"Was unable to fully disconnect a connection!\"\n");
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	return true;
}

bool NU_Client_shutdown(NU_Client_t *client){
	MU_ARG_CHECK(logger, false, client);
	bool fully_shutdown = true;
	MU_COND_RWLOCK_WRLOCK(client->lock, logger);
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		int successful = NU_Connection_disconnect(client->connections[i], logger);
		if(!successful){
			fully_shutdown = false;
			MU_LOG_ERROR(logger, "NU_Connection_shutdown: \"Was unable to fully shutdown a connection!\"\n");
		}
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	return fully_shutdown;
}

bool NU_Client_destroy(NU_Client_t *client){
	MU_ARG_CHECK(logger, false, client);
	bool fully_destroyed = true;
	MU_COND_RWLOCK_WRLOCK(client->lock, logger);
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		int successful = NU_Connection_destroy(client->connections[i], logger);
		if(!successful){
			fully_destroyed = false;
			MU_LOG_ERROR(logger, "NU_Connection_destroy: \"Was unable to fully destroy a connection!\"\n");
		}
	}
	MU_COND_RWLOCK_UNLOCK(client->lock, logger);
	MU_COND_RWLOCK_DESTROY(client->lock, logger);
	return fully_destroyed;
}
