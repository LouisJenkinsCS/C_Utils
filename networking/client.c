#include "networking/client.h"
#include "threading/scoped_locks.h"

struct c_utils_client {
	/// Socket associated with this server.
   	struct c_utils_connection **connections;
   	/// Amount of servers currently connected to.
   	size_t amount_of_connections;
   	/// Lock used for synchronization and thread safety.
   	struct c_utils_scoped_lock *lock;
   	/// Whether or not to initialize locks on everything.
   	bool synchronized;
};

static struct c_utils_logger *logger = NULL;

LOGGER_AUTO_CREATE(logger, "./networking/logs/client.log", "w", LOG_LEVEL_ALL);

static int get_connection_socket(const char *host, unsigned int port, long long int timeout){
	fd_set connect_set;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	char *port_str;
	asprintf(&port_str, "%u", port);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	int retval;
	struct addrinfo *results;
	if((retval = getaddrinfo(host, port_str, &hints, &results))){
		LOG_WARNING(logger, "getaddrinfo: '%s'", gai_strerror(retval));
		free(port_str);
		return -1;
	}
	free(port_str);
	
	// Loop through all potential results to find a valid connection.
	int sockfd = 0, iteration = 0;
	struct addrinfo *current;
	for(current = results; current; current = current->ai_next){
		C_UTILS_TEMP_FAILURE_RETRY(sockfd, socket(current->ai_family, current->ai_socktype, current->ai_protocol));
	    if(sockfd == -1){
	      LOG_VERBOSE(logger, "socket: '%s': Iteration #%d", strerror(errno), ++iteration);
	      continue;
	    }
	    LOG_VERBOSE(logger, "get_connection_socket: 'Received a socket!': Iteration #%d", ++iteration);

	    FD_ZERO(&connect_set);
	    FD_SET(sockfd, &connect_set);
	    
	    C_UTILS_TEMP_FAILURE_RETRY(retval, select(sockfd + 1, &connect_set, NULL, NULL, timeout < 0 ? NULL : &tv));
	    if(retval <= 0){
	    	if(!retval) LOG_VERBOSE(logger, "select: 'Timed out!': Iteration: #%d", ++iteration);
	    	else LOG_VERBOSE(logger, "select: '%s': Iteration: #%d", strerror(errno), ++iteration);
	    	C_UTILS_TEMP_FAILURE_RETRY(retval, close(sockfd));
	    	continue;
	    }
	    
	    C_UTILS_TEMP_FAILURE_RETRY(retval, connect(sockfd, current->ai_addr, current->ai_addrlen));
	    if(retval == -1){
	      LOG_VERBOSE(logger, "connect: '%s'; Iteration #%d", strerror(errno), ++iteration);
	      C_UTILS_TEMP_FAILURE_RETRY(retval, close(sockfd));
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

struct c_utils_client *c_utils_client_create(size_t connection_pool_size, bool init_locks){
	struct c_utils_client *client = calloc(1, sizeof(struct c_utils_client));
	if(!client){
		LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	
	bool mutex_init = false;
	pthread_mutex_t *lock = NULL;

	if(init_locks){
		lock = malloc(sizeof(pthread_mutex_t));
		if(!lock){
			LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		int failure = pthread_mutex_init(lock, NULL);
		if(failure){
			LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(failure));
			goto error;
		}
		
		mutex_init = true;
		client->synchronized = true;
	}
	
	client->lock = SCOPED_LOCK_FROM(lock, logger);
	if(!client->lock){
		LOG_ASSERT(logger, "SCOPED_LOCK_FROM: 'Unable to create scoped lock from mutex!'");
		goto error;
	}
	
	size_t connections_allocated = 0;
	client->amount_of_connections = connection_pool_size ? connection_pool_size : 1;
	client->connections = calloc(client->amount_of_connections, sizeof(struct c_utils_connection *));
	if(!client->connections){
		LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	
	size_t i = 0;
	for(;i < client->amount_of_connections; i++){
		struct c_utils_connection *conn = c_utils_connection_create(init_locks, logger);
		if(!conn){
			LOG_ASSERT(logger, "c_utils_connection_create: 'Was unable to create connection #%d!'", ++i);
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
				int is_destroyed = c_utils_connection_destroy(client->connections[i]);
				if(!is_destroyed){
					LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to destroy a connection'");
				}
			}
			free(client->connections);
		}
		if(client->lock){
			c_utils_scoped_lock_destroy(client->lock);
		} else if (lock) {
			if(mutex_init) {
				pthread_mutex_destroy(lock);
			}
			free(lock);
		}
		if(client){
			free(client);
		}
		return NULL;

}

struct c_utils_connection *c_utils_client_connect(struct c_utils_client *client, const char *ip_addr, unsigned int port, long long int timeout){
	ARG_CHECK(logger, NULL, client, ip_addr, port > 0);
	
	int sockfd = get_connection_socket(ip_addr, port, timeout);
	if(sockfd == -1){
		LOG_WARNING(logger, "get_connection_socket: 'Was unable to form a connection!'");
		return NULL;
	}
	
	bool successful = false;
	SCOPED_LOCK(client->lock) {
		struct c_utils_connection *conn = c_utils_connection_reuse(client->connections, client->amount_of_connections, sockfd, port, ip_addr, logger);
		if(conn){
			LOG_INFO(logger, "Connected to %s on port %u", ip_addr, port);
			return conn;
		}
		
		conn = c_utils_connection_create(client->synchronized, logger);
		if(!conn){
			LOG_ERROR(logger, "c_utils_connection_create: 'Was unable to create connection!'");
			return NULL;
		}
		
		struct c_utils_connection **tmp_connections = realloc(client->connections, sizeof(struct c_utils_connection *) * client->amount_of_connections + 1);
		if(!tmp_connections){
			LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
			return NULL;
		}
		
		client->connections = tmp_connections;
		client->connections[client->amount_of_connections++] = conn;
		successful = c_utils_connection_init(conn, sockfd, port, ip_addr, logger);
	}
	if(!successful){
		LOG_WARNING(logger, "c_utils_connection_init: 'Was unable to initalize client!'");
		return NULL;
	}
	
	LOG_INFO(logger, "Connected to %s on port %u", ip_addr, port);
	return conn;
}

bool c_utils_client_log(struct c_utils_client *client, const char *message, ...){
	ARG_CHECK(logger, false, client, message);
	
	va_list args;
	va_start(args, message);
	
	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		LOG_WARNING(logger, "vsnprintf: '%s'", strerror(errno));
		return false;
	}
	
	LOG_CUSTOM(logger, "CLIENT", message, "%s", buffer);
	return true;
}

bool c_utils_client_disconnect(struct c_utils_client *client, struct c_utils_connection *conn){
	ARG_CHECK(logger, false, client, conn);
	
	SCOPED_LOCK(client->lock) {
		int successful = c_utils_connection_disconnect(conn);
		if(!successful){
			LOG_ERROR(logger, "c_utils_connection_disconnect: 'Was unable to fully disconnect a connection!'");
		}
	}

	return true;
}

bool c_utils_client_shutdown(struct c_utils_client *client){
	ARG_CHECK(logger, false, client);
	
	bool fully_shutdown = true;
	SCOPED_LOCK(client->lock) {
		size_t i = 0;
		for(;i < client->amount_of_connections; i++){
			int successful = c_utils_connection_disconnect(client->connections[i]);
			if(!successful){
				fully_shutdown = false;
				LOG_ERROR(logger, "c_utils_connection_shutdown: 'Was unable to fully shutdown a connection!'");
			}
		}
	}

	return fully_shutdown;
}

bool c_utils_client_destroy(struct c_utils_client *client){
	ARG_CHECK(logger, false, client);
	
	int is_shutdown = c_utils_client_shutdown(client);
	if(!is_shutdown){
		LOG_ERROR(logger, "c_utils_client_shutdown: 'Was unable to shutdown client!'");
		return false;
	}

	SCOPED_LOCK(client->lock) {
		size_t i = 0;
		for(;i < client->amount_of_connections; i++){
			int successful = c_utils_connection_destroy(client->connections[i]);
			if(!successful){
				LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to fully destroy a connection!'");
			}
		}
	}

	c_utils_scoped_lock_destroy(client->lock);
	free(client->connections);
	free(client);

	return true;
}
