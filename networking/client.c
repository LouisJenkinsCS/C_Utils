#include "client.h"

#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "../threading/scoped_lock.h"
#include "../misc/alloc_check.h"
#include "../misc/signal_retry.h"
#include "../misc/argument_check.h"

struct c_utils_client {
	/// Socket associated with this server.
   	struct c_utils_connection **conn_pool;
   	/// Amount of servers currently connected to.
   	size_t conn_pool_size;
   	/// Lock used for synchronization and thread safety.
   	struct c_utils_scoped_lock *lock;
   	/// Whether or not to initialize locks on everything.
   	bool synchronized;
};

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./networking/logs/client.log", "w", C_UTILS_LOG_LEVEL_ALL);

static int get_connection_socket(const char *host, unsigned int port, long long int timeout) {
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
	if ((retval = getaddrinfo(host, port_str, &hints, &results))) {
		C_UTILS_LOG_WARNING(logger, "getaddrinfo: '%s'", gai_strerror(retval));
		free(port_str);
		return -1;
	}
	free(port_str);
	
	// Loop through all potential results to find a valid connection.
	int sockfd = 0, iteration = 0;
	struct addrinfo *current;
	for (current = results; current; current = current->ai_next) {
		C_UTILS_TEMP_FAILURE_RETRY(sockfd, socket(current->ai_family, current->ai_socktype, current->ai_protocol));
	    if (sockfd == -1) {
	      C_UTILS_LOG_VERBOSE(logger, "socket: '%s': Iteration #%d", strerror(errno), ++iteration);
	      continue;
	    }
	    C_UTILS_LOG_VERBOSE(logger, "get_connection_socket: 'Received a socket!': Iteration #%d", ++iteration);

	    FD_ZERO(&connect_set);
	    FD_SET(sockfd, &connect_set);
	    
	    C_UTILS_TEMP_FAILURE_RETRY(retval, select(sockfd + 1, &connect_set, NULL, NULL, timeout < 0 ? NULL : &tv));
	    if (retval <= 0) {
	    	if (!retval)
	    		C_UTILS_LOG_VERBOSE(logger, "select: 'Timed out!': Iteration: #%d", ++iteration);
	    	else
	    		C_UTILS_LOG_VERBOSE(logger, "select: '%s': Iteration: #%d", strerror(errno), ++iteration);
	    	
	    	C_UTILS_TEMP_FAILURE_RETRY(retval, close(sockfd));
	    	continue;
	    }
	    
	    C_UTILS_TEMP_FAILURE_RETRY(retval, connect(sockfd, current->ai_addr, current->ai_addrlen));
	    if (retval == -1) {
	      C_UTILS_LOG_VERBOSE(logger, "connect: '%s'; Iteration #%d", strerror(errno), ++iteration);
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

struct c_utils_client *c_utils_client_create(size_t connection_pool_size, bool synchronized) {
	struct c_utils_client *client;
	C_UTILS_ON_BAD_CALLOC(client, logger, sizeof(*client))
		goto err;

	client->lock = (client->synchronized = synchronized) ? c_utils_scoped_lock_mutex(NULL, logger) : c_utils_scoped_lock_no_op();
	if (!client->lock) {
		C_UTILS_LOG_ERROR(logger, "Was unable to create scoped_lock!");
		goto err_lock;
	}

	size_t conns_allocated = 0;
	client->conn_pool_size = connection_pool_size ? connection_pool_size : 1;
	C_UTILS_ON_BAD_CALLOC(client->conn_pool, logger, sizeof(*client->conn_pool))
		goto err_conn_pool;

	for (size_t i = 0; i < client->conn_pool_size; i++) {
		struct c_utils_connection *conn = c_utils_connection_create(synchronized, logger);
		if (!conn) {
			C_UTILS_LOG_ERROR(logger, "c_utils_connection_create: 'Was unable to create connection #%zu!'", ++i);
			goto err_conn_alloc;
		}

		client->conn_pool[i] = conn;
		conns_allocated++;
	}

	return client;

	/// Deallocate all memory allocated if anything fails!
	err_conn_alloc:
		for(size_t i = 0; i < conns_allocated; i++)
			if(!c_utils_connection_destroy(client->conn_pool[i]))
				C_UTILS_LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to destroy a connection'");
	err_conn_pool:
		c_utils_scoped_lock_destroy(client->lock);
	err_lock:
		free(client);
	err:
		return NULL;
}

struct c_utils_connection *c_utils_client_connect(struct c_utils_client *client, const char *ip_addr, unsigned int port, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, NULL, client, ip_addr, port > 0);
	
	int sockfd = get_connection_socket(ip_addr, port, timeout);
	if (sockfd == -1) {
		C_UTILS_LOG_WARNING(logger, "get_connection_socket: 'Was unable to form a connection!'");
		return NULL;
	}
	
	bool successful = false;
	struct c_utils_connection *conn;
	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(client->lock) {
		conn = c_utils_connection_reuse(client->conn_pool, client->conn_pool_size, sockfd, port, ip_addr, logger);
		if (conn) {
			C_UTILS_LOG_INFO(logger, "Connected to %s on port %u", ip_addr, port);
			return conn;
		}
		
		conn = c_utils_connection_create(client->synchronized, logger);
		if (!conn) {
			C_UTILS_LOG_ERROR(logger, "c_utils_connection_create: 'Was unable to create connection!'");
			return NULL;
		}
		
		C_UTILS_ON_BAD_REALLOC(&client->conn_pool, logger, sizeof(struct c_utils_connection *) * (client->conn_pool_size + 1)) {
			c_utils_connection_destroy(conn);
			return NULL;
		}
		
		client->conn_pool[client->conn_pool_size++] = conn;
		successful = c_utils_connection_init(conn, sockfd, port, ip_addr, logger);
	} // Release Mutex
	
	if (!successful) {
		C_UTILS_LOG_WARNING(logger, "c_utils_connection_init: 'Was unable to initalize client!'");
		return NULL;
	}
	
	C_UTILS_LOG_INFO(logger, "Connected to %s on port %u", ip_addr, port);
	return conn;
}

bool c_utils_client_log(struct c_utils_client *client, const char *message, ...) {
	C_UTILS_ARG_CHECK(logger, false, client, message);
	
	va_list args;
	va_start(args, message);
	
	const int buf_size = 1024;
	char buffer[buf_size];
	if (vsnprintf(buffer, buf_size, message, args) < 0) { 
		C_UTILS_LOG_WARNING(logger, "vsnprintf: '%s'", strerror(errno));
		return false;
	}
	
	C_UTILS_LOG_CUSTOM(logger, "CLIENT", message, "%s", buffer);
	return true;
}

bool c_utils_client_disconnect(struct c_utils_client *client, struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(logger, false, client, conn);
	
	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(client->lock) {
		int successful = c_utils_connection_disconnect(conn);
		if (!successful)  
			C_UTILS_LOG_ERROR(logger, "c_utils_connection_disconnect: 'Was unable to fully disconnect a connection!'");
		
		return successful;
	} // Release Mutex

	C_UTILS_UNACCESSIBLE;
}

bool c_utils_client_shutdown(struct c_utils_client *client) {
	C_UTILS_ARG_CHECK(logger, false, client);
	
	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(client->lock) {
		bool fully_shutdown = true;
		size_t i = 0;
		for (;i < client->conn_pool_size; i++) {
			int successful = c_utils_connection_disconnect(client->conn_pool[i]);
			if (!successful) {
				fully_shutdown = false;
				C_UTILS_LOG_ERROR(logger, "c_utils_connection_shutdown: 'Was unable to fully shutdown a connection!'");
			}
		}

		return fully_shutdown;
	} // Release Mutex

	C_UTILS_UNACCESSIBLE;
}

bool c_utils_client_destroy(struct c_utils_client *client) {
	C_UTILS_ARG_CHECK(logger, false, client);
	
	int is_shutdown = c_utils_client_shutdown(client);
	if (!is_shutdown) {
		C_UTILS_LOG_ERROR(logger, "c_utils_client_shutdown: 'Was unable to shutdown client!'");
		return false;
	}

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(client->lock)
		for (size_t i = 0;i < client->conn_pool_size; i++)
			if (!c_utils_connection_destroy(client->conn_pool[i]))
				C_UTILS_LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to fully destroy a connection!'");

	c_utils_scoped_lock_destroy(client->lock);
	free(client->conn_pool);
	free(client);

	return true;
}
