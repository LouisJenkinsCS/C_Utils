#include <NU_Server.h>

static MU_Logger_t *logger = NULL;

#define MU_LOG_SERVER(message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

__attribute__((constructor)) static void init_logger(void){
	logger = calloc(1, sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("init_logger->malloc: \"%s\"\n", strerror(errno));
		return;
	}
	MU_Logger_Init(logger, "NU_Server.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

/* Server-specific helper functions */

static char *bsock_to_string(NU_Bound_Socket_t *bsock){
	if(!bsock) return NULL;
	MU_Cond_rwlock_rdlock(bsock->lock, logger);
	char *bsock_str;
	asprintf(&bsock_str, "(port: %d, sockfd: %d)", bsock->port, bsock->sockfd);
	MU_Cond_rwlock_unlock(bsock->lock, logger);
	return bsock_str;
}

static int setup_bound_socket(NU_Bound_Socket_t *bsock, const char *ip_addr, unsigned int port, size_t queue_size){
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
	my_addr.sin_addr.s_addr = ip_addr ? inet_addr(ip_addr) : INADDR_ANY;
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

static NU_Bound_Socket_t *reuse_existing_socket(NU_Bound_Socket_t **sockets, size_t size){
	size_t i = 0;
	for(;i < size; i++){
		NU_Bound_Socket_t *bsock = sockets[i];
		MU_Cond_rwlock_wrlock(bsock->lock, logger);
		if(bsock && !bsock->is_bound){
			bsock->is_bound = 1;
			MU_Cond_rwlock_unlock(bsock->lock, logger);
			return bsock;
		}
		MU_Cond_rwlock_unlock(bsock->lock, logger);
	}
	return NULL;
}

static void destroy_bound_socket(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	if(!server || !bsock){
		MU_LOG_ERROR(logger, "destroy_bound_socket: Invalid Arguments=> \"Server: %s;Bound Socket: %s\"\n", server ? "OK!" : "NULL", bsock ? "OK!" : "NULL");
		return;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	MU_Cond_rwlock_wrlock(bsock->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = server->connections[i];
		if(NU_Connection_get_port(conn) == bsock->port){
			int is_disconnected = NU_Connection_disconnect(conn, logger);
			if(!is_disconnected){
				MU_LOG_ERROR(logger, "destroy_bound_socket->NU_Connection_disconnect: \"Was unable to disconnect a connection!\"\n");
			}
		}
	}
	close(bsock->sockfd);
	bsock->is_bound = 0;
	MU_Cond_rwlock_unlock(bsock->lock, logger);
	MU_Cond_rwlock_unlock(server->lock, logger);
}

NU_Server_t *NU_Server_create(size_t connection_pool_size, size_t bsock_pool_size, unsigned char init_locks){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	if(!server){
		MU_LOG_ASSERT(logger, "NU_Server_create->calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	// Keep track of everything allocated. If anything fails to allocate, we will free up all memory allocated in this (very long) block.
	size_t connections_allocated = 0, bsocks_allocated = 0;
	server->data = NU_Atomic_Data_create();
	if(!server->data){
		MU_LOG_ERROR(logger, "NU_Server_create->NU_Collective_Data_create: \"Was unable to allocate atomic data\"\n");
		goto error;
	}
	if(init_locks){
		server->lock = malloc(sizeof(pthread_rwlock_t));
		if(!server->lock){
			MU_LOG_ASSERT(logger, "NU_Server_create->malloc: \"%s\"\n", strerror(errno));
			goto error;
		}
		int failure = pthread_rwlock_init(server->lock, NULL);
		if(failure){
			MU_LOG_ERROR(logger, "NU_Server_create->pthread_rwlock_init: \"%s\"\n", strerror(failure));
			goto error;
		}
	}
	server->amount_of_sockets = bsock_pool_size ? bsock_pool_size : 1;
	server->sockets = calloc(server->amount_of_sockets, sizeof(NU_Bound_Socket_t *));
	if(!server->sockets){
		MU_LOG_ASSERT(logger, "NU_Server_create->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	size_t i = 0;
	for(;i < server->amount_of_sockets; i++){
		NU_Bound_Socket_t *bsock = NU_Bound_Socket_create(init_locks, logger);
		if(!bsock){
			MU_LOG_ERROR(logger, "NU_Server_create->NU_Bound_Socket_create: \"Was unable to create bound socket #%d\"\n", ++i);
			goto error;
		}
		server->sockets[i] = bsock;
		bsocks_allocated++;
	}
	server->amount_of_connections = connection_pool_size ? connection_pool_size : 1;
	server->connections = calloc(server->amount_of_connections, sizeof(NU_Connection_t *));
	if(!server->connections){
		MU_LOG_ASSERT(logger, "NU_Server_create->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = NU_Connection_create(NU_SERVER, init_locks, logger);
		if(!conn){
			MU_LOG_ASSERT(logger, "NU_Server_create->NU_Connection_create: \"Was unable to create connection #%d!\"\n", ++i);
			goto error;
		}
		server->connections[i] = conn;
		connections_allocated++;
	}
	return server;
	/// Deallocate all memory allocated if something were to fail!
	error:
		if(server->connections){
			size_t i = 0;
			for(;i < connections_allocated; i++){
				int is_destroyed = NU_Connection_destroy(server->connections[i], logger);
				if(!is_destroyed){
					MU_LOG_ERROR(logger, "NU_Server_create->NU_Connection_destroy: \"Was unable to destroy a connection\"\n");
				}

			}
			free(server->connections);
		}
		if(server->sockets){
			size_t i = 0;
			for(;i < bsocks_allocated; i++){
				int is_destroyed = NU_Bound_Socket_destroy(server->sockets[i], logger);
				if(!is_destroyed){
					MU_LOG_ERROR(logger, "NU_Server_create->NU_Bound_Socket_destroy: \"Was unable to destroy a socket!\"\n");
				}
			}
			free(server->sockets);
		}
		if(init_locks){
			MU_Cond_rwlock_destroy(server->lock, logger);
		}
		if(server->data){
			free(server->data);
		}
		if(server){
			free(server);
		}
		return NULL;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, const char *ip_addr, unsigned int port, size_t queue_size, unsigned char init_locks){
	if(!server || !port || !queue_size || !ip_addr){
		MU_LOG_ERROR(logger, "NU_Server_bind: Invalid Arguments=> \"Server: %s;Port > 0: %s; Queue Size > 0: %s; IP Address: %s\n",
			server ? "OK!" : "NULL", port ? "OK!" : "NO!", queue_size ? "OK!" : "NO!", ip_addr ? "OK!" : "NULL");
		return NULL;
	}
	MU_Cond_rwlock_wrlock(server->lock, logger);
	NU_Bound_Socket_t *bsock = reuse_existing_socket(server->sockets, server->amount_of_sockets);
	if(!bsock){
		bsock = calloc(1, sizeof(NU_Bound_Socket_t));
		if(!bsock){
			MU_LOG_ASSERT(logger, "NU_Server_bind->calloc: \"%s\"\n", strerror(errno));
			MU_Cond_rwlock_unlock(server->lock, logger);
			return NULL;
		}
		if(init_locks){
			bsock->lock = malloc(sizeof(pthread_rwlock_t));
			if(!bsock->lock){
				MU_LOG_ASSERT(logger, "NU_Server_bind->calloc: \"%s\"\n", strerror(errno));
				free(bsock);
				return NULL;
			}
		}
		int is_successful = MU_Cond_rwlock_init(bsock->lock, NULL, logger);
		if(!is_successful){
			free(bsock->lock);
			free(bsock);
			return NULL;
		}
		NU_Connection_t **tmp_sockets = realloc(server->sockets, sizeof(NU_Bound_Socket_t *) * server->amount_of_sockets + 1);
		if(!tmp_sockets){
			MU_LOG_ASSERT(logger, "NU_Server_bind->realloc: \"%s\"\n", strerror(errno));
			MU_Cond_rwlock_unlock(server->lock, logger);
			MU_Cond_rwlock_destroy(bsock->lock, logger);
			return NULL;
		}
		server->sockets = tmp_sockets;
		server->sockets[server->amount_of_sockets++] = bsock;
	}
	if(!setup_bound_socket(bsock, ip_addr, port, queue_size)){
		MU_LOG_WARNING(logger, "NU_Server_bind->setup_bound_socket: \"was unable to setup bsock\"\n");
		MU_Cond_rwlock_unlock(server->lock, logger);
		return NULL;
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	return bsock;
}

int NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	if(!server || !bsock){
		MU_LOG_ERROR(logger, "NU_Server_unbind: Invalid Arguments=> \"Server: %s;Bound Socket: %s\"\n", server ? "OK!" : "NULL", bsock ? "OK!" : "NULL");
		return 0;
	}
	// Even though the server isn't directly modified, in order to send data to a connection, the readlock must be acquired, hence this will prevent
	// any connections from sending to a shutting down socket. Also the socket will not shutdown until the readlocks are released.
	MU_Cond_rwlock_wrlock(server->lock, logger);
	MU_Cond_rwlock_wrlock(bsock->lock, logger);
	if(shutdown(bsock->sockfd, SHUT_RDWR) == -1){
		MU_LOG_ERROR(logger, "NU_Server_unbind->shutdown: \"%s\"\n", strerror(errno));
		MU_Cond_rwlock_unlock(bsock->lock, logger);
		MU_Cond_rwlock_unlock(server->lock, logger);
		return 0;
	}
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = server->connections[i];
		if(NU_Connection_get_port(conn) == bsock->port){
			int is_disconnected = NU_Connection_disconnect(conn, logger);
			if(!is_disconnected){
				MU_LOG_ERROR(logger, "NU_Server_unbind->NU_Connection_disconnect: \"Was unable to disconnect a connection!\"\n");
			}
		}
		MU_Cond_rwlock_unlock(bsock->lock, logger);
		MU_Cond_rwlock_unlock(server->lock, logger);
	}
	return 1;
}

NU_Connection_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *bsock, unsigned char init_locks, unsigned int timeout){
	if(!server || !bsock){
		MU_LOG_ERROR(logger, "NU_Server_accept: Invalid Arguments=> \"Server: %s;Bound Socket: %s\"\n", server ? "OK!" : "NULL", bsock ? "OK!" : "NULL");
		return NULL;
	}
	char ip_addr[INET_ADDRSTRLEN];
	MU_Cond_rwlock_rdlock(bsock->lock, logger);
	unsigned int port = bsock->port;
	int sockfd = NU_timed_accept(bsock->sockfd, ip_addr, timeout, logger);
	if(sockfd == -1){
		MU_LOG_INFO(logger, "NU_Server_accept->accept: \"Was unable to accept a server!\"\n");
		return NULL;
	}
	NU_Cond_rwlock_unlock(bsock->lock, logger);
	MU_Cond_rwlock_rdlock(server->lock, logger);
	NU_Connection_t *conn = NU_Connection_reuse(server->connections, server->amount_of_connections, sockfd, port, ip_addr, logger);
	if(conn){
		MU_Cond_rwlock_unlock(server->lock, logger);
		MU_LOG_INFO(logger, "%s connected to port %d\n", ip_addr, bsock->port);
		return conn;
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	MU_Cond_rwlock_wrlock(server->lock, logger);
	// If the NULL is returned, then a connection could not be reused, hence we initialize a new one below.
	conn = NU_Connection_create(NU_SERVER, init_locks, logger);
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Server_accept->NU_Connection_create: \"Was unable to create a connection!\"\n");
		MU_Cond_rwlock_unlock(server->lock, logger);
		return NULL;
	}
	NU_Connection_t **tmp_connections = realloc(server->connections, sizeof(NU_Connection_t *) * server->amount_of_connections + 1);
	if(!tmp_connections){
		MU_LOG_ASSERT(logger, "NU_Server_accept->realloc: \"%s\"\n", strerror(errno));
		MU_Cond_rwlock_unlock(server->lock, logger);
		return NULL;
	}
	server->connections = tmp_connections;
	server->connections[server->amount_of_connections++] = conn;
	int is_initialized = NU_Connection_init(conn, sockfd, ip_addr, bsock->port, logger);
	MU_Cond_rwlock_unlock(server->lock, logger);
	if(!is_initialized){
		NU_LOG_ERROR(logger, "NU_Server_accept->NU_Connection_init: \"Was unable to initialize a connection!\"\n");
		int is_closed = TEMP_FAILURE_RETRY(close(sockfd));
		if(!is_closed){
			MU_LOG_ERROR(logger, "NU_Server_accept->close: \"%s\"\n", strerror(errno));
		}
		return NULL;
	}
	MU_LOG_INFO(logger, "%s connected to port %d\n", ip_addr, bsock->port);
	return conn;
}

size_t NU_Server_send(NU_Server_t *server, NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout){
	if(!server || !conn || conn->type != NU_SERVER){
		MU_LOG_ERROR(logger, "NU_Server_send: Invalid Arguments=> \"Server: %s;Connection: %s;Connection-Type: %s\"\n",
			server ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	size_t result = NU_Connection_send(conn, buffer, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total Sent: %zu, Buffer Size: %zu\n", result, buf_size);
	NU_Atomic_Data_increment_sent(server->data, result);
	if(result != buf_size){
		MU_LOG_WARNING(logger, "NU_Server_send->NU_Connection_send: \"Was unable to send %zu bytes to %s!\"\n", buf_size - result, NU_Connection_get_ip_addr(conn));
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	return result;
}

size_t NU_Server_receive(NU_Server_t *server, NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout){
	if(!server || !conn || conn->type != NU_SERVER){
		MU_LOG_ERROR(logger, "NU_Server_receive: Invalid Arguments=> \"Server: %s;Connection: %s;Connection-Type: %s\"\n",
			server ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	size_t result = NU_Connection_receive(conn, buf_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu\n", result, buf_size);
	if(!result){
		MU_LOG_WARNING(logger, "NU_Server_receive->NU_Connection_receive: \"Was unable to receive from %s!\"\n", NU_Connection_get_ip_addr(conn));
		return 0;
	}
	NU_Atomic_Data_increment_received(server->data, result);
	MU_Cond_rwlock_unlock(server->lock, logger);
	return result;
}

size_t NU_Server_send_file(NU_Server_t *server, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	if(!server || !conn || !file || conn->type != NU_SERVER){
		MU_LOG_ERROR(logger, "NU_Server_send_file: Invalid Arguments=> \"Server: %s;File: %s;Connection: %s;Connection-Type: %s\"\n",
			server ? "OK!" : "NULL", file ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	// Obtain the file size to aid in determining whether or not the send was successful or not.
	struct stat file_stats
;	int file_fd = fileno(file);
	if(file_fd == -1){
		MU_LOG_WARNING(logger, "NU_Server_send_file->fileno: \"%s\"\n", strerror(errno));
	}
	size_t file_size = 0;
	if(fstat(file_fd, &file_stats) == -1){
		MU_LOG_WARNING(logger, "NU_Server_send_file->fstat: \"%s\"\n", strerror(errno));
	}
	else {
		file_size = file_stats.st_size;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	size_t total_sent = NU_Connection_send_file(conn, file, buf_size, timeout, logger);
	// Note that if the file size is zero, meaning that there was an error with fstat, it will skip this check.
	if(!total_sent){
		MU_LOG_WARNING(logger, "NU_Server_send_file->NU_Connection_send_file: \"No data was sent to %s\"\n", NU_Connection_get_ip_addr(conn));
		MU_Cond_rwlock_unlock(server->lock, logger);
		return 0;
	}
	else if(file_size && file_size != total_sent){ 
		MU_LOG_WARNING(logger, "NU_Server_send_file->NU_Connection_send_file: \"File Size is %zu, but only sent %zu to %s\"\n",
			file_size, total_sent, NU_Connection_get_ip_addr(conn));
	}
	NU_Atomic_Data_increment_sent(server->data, total_sent);
	MU_Cond_rwlock_unlock(server->lock, logger);
	MU_LOG_VERBOSE(logger, "Sent file of total size %zu to %s!\n", total_sent, NU_Connection_get_ip_addr(conn));
	return total_sent;
}

size_t NU_Server_receive_to_file(NU_Server_t *server, NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout){
	if(!server || !conn || conn->type != NU_SERVER){
		MU_LOG_ERROR(logger, "NU_Server_receive_to_file: Invalid Arguments=> \"Server: %s;Connection: %s;Connection-Type: %s\"\n",
			server ? "OK!" : "NULL", conn ? "OK!" : "NULL", conn ? NU_Connection_Type_to_string(conn->type) : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	size_t total_received = NU_Connection_receive_to_file(conn, file, buf_size, timeout, logger);
	if(!total_received) MU_LOG_WARNING(logger, "NU_Server_receive_to_file->NU_Connection_receive_to_file: \"Was unable to receive file from %s\"\n", NU_Connection_get_ip_addr(conn));
	NU_Atomic_Data_increment_received(server->data, total_received);
	MU_Cond_rwlock_unlock(server->lock, logger);
	MU_LOG_VERBOSE(logger, "Received file of total size %zu from %s!\n", total_received, NU_Connection_get_ip_addr(conn));
	return total_received;
}

NU_Connection_t **NU_Server_select_receive(NU_Server_t *server, NU_Connection_t **connections, size_t *size, unsigned int timeout){
	if(!server){
		MU_LOG_ERROR(logger, "NU_Server_select_receive: Invalid Argument=> \"Server: %s\"\n", server ? "OK!" : "NULL");
		*size = 0;
		return NULL;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	NU_Connection_t **ready_connections = NU_select_receive_connections(connections, size, timeout, logger);
	MU_Cond_rwlock_unlock(server->lock, logger);
	return ready_connections;
}

NU_Connection_t **NU_Server_select_send(NU_Server_t *server, NU_Connection_t **connections, size_t *size, unsigned int timeout){
	if(!server){
		MU_LOG_ERROR(logger, "NU_Server_select_send: Invalid Argument=> \"Server: %s\"\n", server ? "OK!" : "NULL");
		*size = 0;
		return NULL;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	NU_Connection_t **ready_connections = NU_select_send_connections(connections, size, timeout, logger);
	MU_Cond_rwlock_unlock(server->lock, logger);
	return ready_connections;
}

char *NU_Server_about(NU_Server_t *server){
	if(!server){
		MU_LOG_ERROR(logger, "NU_Server_about: Invalid Argument=> \"Server: %s\"\n", server ? "OK!" : "NULL");
		return NULL;
	}
	MU_Cond_rwlock_rdlock(server->lock, logger);
	char *server_str = "Connections: { ", *old_server_str;
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		old_server_str = server_str;
		asprintf(&server_str, "%s%s%s", server_str, NU_Connection_to_string(server->connections[i]), i < server->amount_of_connections - 1 ? ", " : "");
		// Since server_str is originally a string literal, freeing it would cause a segmentation fault, so we only skip the first one.
		if(i > 0){
			free(old_server_str);
		}
	}
	old_server_str = server_str;
	asprintf(&server_str, "%s }\nBound Sockets: { ", server_str);
	free(old_server_str);
	for(i = 0;server->amount_of_sockets;i++){
		old_server_str = server_str;
		asprintf(&server_str, "%s%s%s", server_str, bsock_to_string(server->sockets[i]), i < server->amount_of_sockets - 1 ? ", " : "");
		free(old_server_str);
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	return server_str;
}

int NU_Server_log(NU_Server_t *server, const char *message, ...){
	if(!server || !message){
		MU_LOG_ERROR(logger, "NU_Server_log: Invalid Arguments=> \"Server: %s;Message: %s\"\n", server ? "OK!" : "NULL", message ? "OK!" : "NULL");
		return 0;
	}
	va_list args;
	va_start(args, message);
	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		MU_LOG_WARNING(logger, "NU_Server_log->vsnprintf: \"%s\"\n", strerror(errno));
		return 0;
	}
	MU_LOG_SERVER("%s", buffer);
	return 1;
}

int NU_Server_disconnect(NU_Server_t *server, NU_Connection_t *connection){
	if(!server){
		MU_LOG_ERROR(logger, "NU_Server_disconnect: Invalid Argument=> \"Server: %s\"\n", server ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_wrlock(server->lock, logger);
	int successful = NU_Connection_disconnect(connection, logger);
	if(!successful){
		MU_LOG_ERROR(logger, "NU_Server_disconnect->NU_Connection_disconnect: \"Was unable to fully disconnect a connection!\"\n");
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	return 1;
}

int NU_Server_shutdown(NU_Server_t *server){
	if(!server){
		MU_LOG_ERROR(logger, "NU_Server_shutdown: Invalid Arguments=> \"Server: %s\"\n", server ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_wrlock(server->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		int successful = NU_Connection_disconnect(server->connections[i], logger);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Server_shutdown->NU_Connection_shutdown: \"Was unable to fully shutdown a connection!\"\n");
		}
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	return 1;
}

int NU_Server_destroy(NU_Server_t *server){
	if(!server){
		MU_LOG_ERROR(logger, "NU_Server_destroy: Invalid Arguments=> \"Server: %s\"\n", server ? "OK!" : "NULL");
		return 0;
	}
	int fully_destroyed = 1;
	MU_Cond_rwlock_wrlock(server->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		int successful = NU_Connection_destroy(server->connections[i], logger);
		if(!successful){
			fully_destroyed = 0;
			MU_LOG_ERROR(logger, "NU_Server_destroy->NU_Connection_destroy: \"Was unable to fully destroy a connection!\"\n");
		}
	}
	MU_Cond_rwlock_unlock(server->lock, logger);
	if(server->lock){
		int successful = pthread_rwlock_destroy(server->lock);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Server_destroy->pthread_rwlock_destroy: \"%s\"\n", strerror(successful));
			fully_destroyed = 0;
		}
		else{
			// Note that the lock is only freed if it successfull deallocates it. If EBUSY ends up being returned, then it's only a memory leak and not
			// undefined behavior.
			free(server->lock);
		}
	}
	free(server->data);
	return fully_destroyed;
}
