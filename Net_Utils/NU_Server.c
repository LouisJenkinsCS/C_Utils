#include <NU_Server.h>

static MU_Logger_t *logger = NULL;

#define MU_LOG_SERVER(message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("NU_Server.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
	free(logger);
}

/* Server-specific helper functions */

static int NU_Bound_Socket_setup(NU_Bound_Socket_t *bsock, size_t queue_size, unsigned int port, const char *ip_addr){
	if(!bsock) return 0;
	int flag = 1;
	struct sockaddr_in my_addr;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		MU_LOG_ERROR(logger, "socket: '%s'", strerror(errno));
		return 0;
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
		MU_LOG_ERROR(logger, "setsockopt: '%s'", strerror(errno));
		goto error;
	}	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = ip_addr ? inet_addr(ip_addr) : INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		MU_LOG_ERROR(logger, "bind: '%s'", strerror(errno));
		goto error;
	}
	if(listen(sockfd, queue_size) == -1){
		MU_LOG_ERROR(logger, "listen: '%s'", strerror(errno));
		goto error;
	}
	bsock->port = port;
	bsock->sockfd = sockfd;
	bsock->is_bound = 1;
	return 1;

	error:
		if(sockfd != -1){
			int close_failed;
			MU_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
			if(close_failed){
				MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
			}
		}
		return 0;
}

static NU_Bound_Socket_t *NU_Bound_Socket_create(bool init_locks){
	NU_Bound_Socket_t *bsock = calloc(1, sizeof(NU_Bound_Socket_t));
	if(!bsock){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	if(init_locks){
		bsock->lock = malloc(sizeof(pthread_rwlock_t));
		if(!bsock->lock){
			MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
	}
	int is_initialized = 1;
	MU_COND_RWLOCK_INIT(bsock->lock, NULL, is_initialized, logger);
	if(!is_initialized){
		goto error;
	}
	return bsock;

	error:
		if(bsock){
			MU_COND_RWLOCK_DESTROY(bsock->lock, logger);
			free(bsock);
		}
		return NULL;
}

static NU_Bound_Socket_t *NU_Bound_Socket_reuse(NU_Bound_Socket_t **sockets, size_t queue_size, size_t size, unsigned int port, const char *ip_addr){
	size_t i = 0;
	for(;i < size; i++){
		NU_Bound_Socket_t *bsock = sockets[i];
		MU_COND_RWLOCK_WRLOCK(bsock->lock, logger);
		if(!bsock->is_bound){
			if(!NU_Bound_Socket_setup(bsock, queue_size, port, ip_addr)){
				MU_LOG_WARNING(logger, "NU_Bound_Socket_setup: 'Was unable to setup bound socket!'");
				MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
				return NULL;
			}
			MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
			return bsock;
		}
		MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
	}
	return NULL;
}

static int NU_Bound_Socket_destroy(NU_Bound_Socket_t *bsock){
	if(!bsock) return 0;
	int close_failed = 0;
	if(bsock->is_bound){
		MU_TEMP_FAILURE_RETRY(close_failed, close(bsock->sockfd));
		if(close_failed){
			MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
		}
	}
	MU_COND_RWLOCK_DESTROY(bsock->lock, logger);
	free(bsock);
	MU_LOG_VERBOSE(logger, "Destroyed a bound socket!");
	return close_failed == 0;
}

static int NU_Bound_Socket_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	MU_COND_RWLOCK_WRLOCK(bsock->lock, logger);
	unsigned int port = bsock->port;
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = server->connections[i];
		if(NU_Connection_get_port(conn, logger) == port && NU_Connection_in_use(conn, logger)){
			int is_disconnected = NU_Connection_disconnect(conn, logger);
			if(!is_disconnected){
				MU_LOG_ERROR(logger, "NU_Connection_disconnect: 'Was unable to disconnect a connection!'");
			}
		}
	}
	int close_failed;
	MU_TEMP_FAILURE_RETRY(close_failed, close(bsock->sockfd));
	if(close_failed){
		MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
	}
	bsock->is_bound = 0;
	MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
	MU_LOG_INFO(logger, "Unbound from port %u!", port);
	return 1;
}

NU_Server_t *NU_Server_create(size_t connection_pool_size, size_t bsock_pool_size, bool init_locks){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	if(!server){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	server->is_threaded = init_locks;
	// Keep track of everything allocated. If anything fails to allocate, we will free up all memory allocated in this (very long) block.
	size_t connections_allocated = 0, bsocks_allocated = 0;
	if(init_locks){
		server->lock = malloc(sizeof(pthread_rwlock_t));
		if(!server->lock){
			MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		int failure = pthread_rwlock_init(server->lock, NULL);
		if(failure){
			MU_LOG_ERROR(logger, "pthread_rwlock_init: '%s'", strerror(failure));
			goto error;
		}
	}
	server->amount_of_sockets = bsock_pool_size ? bsock_pool_size : 1;
	server->sockets = calloc(server->amount_of_sockets, sizeof(NU_Bound_Socket_t *));
	if(!server->sockets){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	size_t i = 0;
	for(;i < server->amount_of_sockets; i++){
		NU_Bound_Socket_t *bsock = NU_Bound_Socket_create(init_locks);
		if(!bsock){
			MU_LOG_ERROR(logger, "NU_Bound_Socket_create: 'Was unable to create bound socket #%zu'", ++i);
			goto error;
		}
		bsock->is_bound = 0;
		server->sockets[i] = bsock;
		bsocks_allocated++;
	}
	server->amount_of_connections = connection_pool_size ? connection_pool_size : 1;
	server->connections = calloc(server->amount_of_connections, sizeof(NU_Connection_t *));
	if(!server->connections){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = NU_Connection_create(init_locks, logger);
		if(!conn){
			MU_LOG_ASSERT(logger, "NU_Connection_create: 'Was unable to create connection #%zu!'", ++i);
			goto error;
		}
		server->connections[i] = conn;
		connections_allocated++;
	}
	return server;

	/// Deallocate all memory allocated if something were to fail!
	error:
		if(server){
			if(server->connections){
			size_t i = 0;
			for(;i < connections_allocated; i++){
				int is_destroyed = NU_Connection_destroy(server->connections[i], logger);
				if(!is_destroyed){
					MU_LOG_ERROR(logger, "NU_Connection_destroy: 'Was unable to destroy a connection!'");
				}

			}
			free(server->connections);
			}
			if(server->sockets){
				size_t i = 0;
				for(;i < bsocks_allocated; i++){
					int is_destroyed = NU_Bound_Socket_destroy(server->sockets[i]);
					if(!is_destroyed){
						MU_LOG_ERROR(logger, "NU_Bound_Socket_destroy: 'Was unable to destroy a socket!'");
					}
				}
				free(server->sockets);
			}
			if(init_locks){
				MU_COND_RWLOCK_DESTROY(server->lock, logger);
			}
			free(server);
		}
		return NULL;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, size_t queue_size, unsigned int port, const char *ip_addr){
	MU_ARG_CHECK(logger, NULL, server, port > 0, queue_size > 0, ip_addr);
	MU_COND_RWLOCK_RDLOCK(server->lock, logger);
	NU_Bound_Socket_t *bsock = NU_Bound_Socket_reuse(server->sockets, server->amount_of_sockets, queue_size, port, ip_addr);
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	if(bsock){
		MU_LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr, port);
		return bsock;
	}
	MU_COND_RWLOCK_WRLOCK(server->lock, logger);
	bsock = NU_Bound_Socket_create(server->is_threaded);
	if(!bsock){
		MU_LOG_ERROR(logger, "NU_Bound_Socket_create: 'Was unable to create a bound socket!'");
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		return NULL;
	}
	NU_Bound_Socket_t **tmp_sockets = realloc(server->sockets, sizeof(NU_Bound_Socket_t *) * server->amount_of_sockets + 1);
	if(!tmp_sockets){
		MU_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		NU_Bound_Socket_destroy(bsock);
		return NULL;
	}
	server->sockets = tmp_sockets;
	server->sockets[server->amount_of_sockets++] = bsock;
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	MU_LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr, port);
	return bsock;
}

bool NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	MU_ARG_CHECK(logger, false, server, bsock);
	// Even though the server isn't directly modified, in order to send data to a connection, the readlock must be acquired, hence this will prevent
	// any connections from sending to a shutting down socket. Also the socket will not shutdown until the readlocks are released.
	MU_COND_RWLOCK_WRLOCK(server->lock, logger);
	MU_COND_RWLOCK_WRLOCK(bsock->lock, logger);
	if(shutdown(bsock->sockfd, SHUT_RDWR) == -1){
		MU_LOG_ERROR(logger, "shutdown: '%s'", strerror(errno));
		MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		return 0;
	}
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = server->connections[i];
		if(NU_Connection_get_port(conn, logger) == bsock->port){
			int is_disconnected = NU_Connection_disconnect(conn, logger);
			if(!is_disconnected){
				MU_LOG_ERROR(logger, "NU_Connection_disconnect: 'Was unable to disconnect a connection!'");
			}
		}
		MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	}
	MU_LOG_INFO(logger, "Unbound from port %u!", bsock->port);
	return 1;
}

NU_Connection_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *bsock, unsigned int timeout){
	MU_ARG_CHECK(logger, NULL, server, bsock);
	char ip_addr[INET_ADDRSTRLEN];
	MU_COND_RWLOCK_RDLOCK(bsock->lock, logger);
	unsigned int port = bsock->port;
	int sockfd = NU_timed_accept(bsock->sockfd, ip_addr, timeout, logger);
	if(sockfd == -1){
		MU_LOG_INFO(logger, "accept: 'Was unable to accept a server!'");
		return NULL;
	}
	MU_COND_RWLOCK_UNLOCK(bsock->lock, logger);
	MU_COND_RWLOCK_RDLOCK(server->lock, logger);
	NU_Connection_t *conn = NU_Connection_reuse(server->connections, server->amount_of_connections, sockfd, port, ip_addr, logger);
	if(conn){
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		MU_LOG_INFO(logger, "%s connected to port %d", ip_addr, bsock->port);
		return conn;
	}
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	MU_COND_RWLOCK_WRLOCK(server->lock, logger);
	// If the NULL is returned, then a connection could not be reused, hence we initialize a new one below.
	conn = NU_Connection_create(server->is_threaded, logger);
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_create: 'Was unable to create a connection!'");
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		return NULL;
	}
	NU_Connection_t **tmp_connections = realloc(server->connections, sizeof(NU_Connection_t *) * server->amount_of_connections + 1);
	if(!tmp_connections){
		MU_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		return NULL;
	}
	server->connections = tmp_connections;
	server->connections[server->amount_of_connections++] = conn;
	int is_initialized = NU_Connection_init(conn, sockfd, bsock->port, ip_addr, logger);
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	if(!is_initialized){
		MU_LOG_ERROR(logger, "NU_Connection_init: 'Was unable to initialize a connection!'");
		int close_failed;
		MU_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
		if(close_failed){
			MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
		}
		return NULL;
	}
	MU_LOG_INFO(logger, "%s connected to port %d", ip_addr, bsock->port);
	return conn;
}

size_t NU_Server_send(NU_Server_t *server, NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, server, conn, buffer, buf_size > 0);
	MU_COND_RWLOCK_RDLOCK(server->lock, logger);
	size_t result = NU_Connection_send(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total Sent: %zu, Buffer Size: %zu", result, buf_size);
	if(result != buf_size){
		MU_LOG_WARNING(logger, "NU_Connection_send: 'Was unable to send %zu bytes to %s!'", buf_size - result, NU_Connection_get_ip_addr(conn, logger));
	}
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	return result;
}

size_t NU_Server_receive(NU_Server_t *server, NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, server, conn, buffer, buf_size > 0);
	MU_COND_RWLOCK_RDLOCK(server->lock, logger);
	size_t result = NU_Connection_receive(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu", result, buf_size);
	if(!result){
		MU_LOG_WARNING(logger, "NU_Connection_receive: 'Was unable to receive from %s!'", NU_Connection_get_ip_addr(conn, logger));
		return 0;
	}
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	return result;
}

size_t NU_Server_send_file(NU_Server_t *server, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, server, conn, file, buf_size > 0);
	const char *ip_addr = NU_Connection_get_ip_addr(conn, logger);
	// Obtain the file size to aid in determining whether or not the send was successful or not.
	struct stat file_stats
;	int file_fd = fileno(file);
	if(file_fd == -1){
		MU_LOG_WARNING(logger, "fileno: '%s'", strerror(errno));
	}
	size_t file_size = 0;
	if(fstat(file_fd, &file_stats) == -1){
		MU_LOG_WARNING(logger, "fstat: '%s'", strerror(errno));
	}
	else {
		file_size = file_stats.st_size;
	}
	MU_COND_RWLOCK_RDLOCK(server->lock, logger);
	size_t total_sent = NU_Connection_send_file(conn, file, buf_size, timeout, logger);
	// Note that if the file size is zero, meaning that there was an error with fstat, it will skip this check.
	if(!total_sent){
		MU_LOG_WARNING(logger, "NU_Connection_send_file: 'No data was sent to %s'", ip_addr);
		MU_COND_RWLOCK_UNLOCK(server->lock, logger);
		return 0;
	}
	else if(file_size && file_size != total_sent){ 
		MU_LOG_WARNING(logger, "NU_Connection_send_file: 'File Size is %zu, but only sent %zu to %s'",
			file_size, total_sent, ip_addr);
	}
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	MU_LOG_VERBOSE(logger, "Sent file of total size %zu to %s!", total_sent, ip_addr);
	return total_sent;
}

size_t NU_Server_receive_file(NU_Server_t *server, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	MU_ARG_CHECK(logger, 0, server, conn, file, buf_size > 0);
	const char *ip_addr = NU_Connection_get_ip_addr(conn, logger);
	MU_COND_RWLOCK_RDLOCK(server->lock, logger);
	size_t total_received = NU_Connection_receive_file(conn, file, buf_size, timeout, logger);
	if(!total_received) MU_LOG_WARNING(logger, "NU_Connection_receive_to_file: 'Was unable to receive file from %s'", ip_addr);
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	MU_LOG_VERBOSE(logger, "Received file of total size %zu from %s!", total_received, ip_addr);
	return total_received;
}

bool NU_Server_log(NU_Server_t *server, const char *message, ...){
	MU_ARG_CHECK(logger, false, server, message);
	va_list args;
	va_start(args, message);
	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		MU_LOG_WARNING(logger, "vsnprintf: '%s'", strerror(errno));
		return 0;
	}
	MU_LOG_SERVER("%s", buffer);
	return 1;
}

bool NU_Server_disconnect(NU_Server_t *server, NU_Connection_t *conn){
	MU_ARG_CHECK(logger, false, server, conn);
	int successful = NU_Connection_disconnect(conn, logger);
	if(!successful){
		MU_LOG_ERROR(logger, "NU_Connection_disconnect: 'Was unable to fully disconnect a connection!'");
	}
	MU_LOG_INFO(logger, "Disconnected from %s on port %u!", NU_Connection_get_ip_addr(conn, logger), NU_Connection_get_port(conn, logger));
	return 1;
}

bool NU_Server_shutdown(NU_Server_t *server){
	MU_ARG_CHECK(logger, false, server);
	MU_COND_RWLOCK_WRLOCK(server->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_sockets; i++){
		int successful = NU_Bound_Socket_unbind(server, server->sockets[i]);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Bound_Socket_unbind: 'Was unable to fully unbind a socket!'");
		}
	}
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	return 1;
}

bool NU_Server_destroy(NU_Server_t *server){
	MU_ARG_CHECK(logger, false, server);
	int is_shutdown = NU_Server_shutdown(server);
	if(!is_shutdown){
		MU_LOG_ERROR(logger, "NU_Server_shutdown: 'Was unable to shutdown server!'");
		return 0;
	}
	MU_COND_RWLOCK_WRLOCK(server->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		int successful = NU_Connection_destroy(server->connections[i], logger);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Connection_destroy: 'Was unable to fully destroy a connection!'");
		}
	}
	for(i = 0;i < server->amount_of_sockets; i++){
		int successful = NU_Bound_Socket_destroy(server->sockets[i]);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Bound_Socket_destroy: 'Was unable to fully destroy a bound socket!'");
		}
	}
	MU_COND_RWLOCK_UNLOCK(server->lock, logger);
	MU_COND_RWLOCK_DESTROY(server->lock, logger);
	free(server->connections);
	free(server->sockets);
	free(server);
	return 1;
}
