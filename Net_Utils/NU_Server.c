#include <NU_Server.h>
#include <MU_Retry.h>
#include <MU_Cond_Locks.h>
#include <MU_Arg_Check.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;

#define MU_LOG_SERVER(message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

<<<<<<< HEAD
__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Net_Utils/Logs/NU_Server.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}
=======
MU_LOGGER_AUTO_CREATE(logger, "./Net_Utils/Logs/NU_Server.log", "w", MU_ALL);
>>>>>>> development

/* Server-specific helper functions */

static int timed_accept(int sockfd, char *ip_addr, long long int timeout){
   int accepted = 0;
   fd_set can_accept;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   FD_ZERO(&can_accept);
   FD_SET(sockfd, &can_accept);
   MU_TEMP_FAILURE_RETRY(accepted, select(sockfd + 1, &can_accept, NULL, NULL, timeout < 0 ? NULL : &tv));
   if(accepted <= 0){
      if(!accepted) MU_LOG_VERBOSE(logger, "select: 'Timed out!'");
      else MU_LOG_ERROR(logger, "select: '%s'", strerror(errno));
      return -1;
   }
   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   MU_TEMP_FAILURE_RETRY(accepted, accept(sockfd, (struct sockaddr *)&addr, &size));
   if(accepted == -1){
      MU_LOG_ERROR(logger, "accept: '%s'", strerror(errno));
      return -1;
   }
   if(ip_addr){
      if(!inet_ntop(AF_INET, &addr, ip_addr , INET_ADDRSTRLEN)) MU_LOG_WARNING(logger, "inet_ntop: '%s'", strerror(errno));
   }
   return accepted;
}

static bool bsock_setup(NU_Bound_Socket_t *bsock, size_t queue_size, unsigned int port, const char *ip_addr){
	if(!bsock) return false;
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
<<<<<<< HEAD
	}	
=======
	}
>>>>>>> development
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
	return true;

	error:
		if(sockfd != -1){
			int close_failed;
			MU_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
			if(close_failed){
				MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
			}
		}
		return false;
}

static NU_Bound_Socket_t *bsock_create(void){
	NU_Bound_Socket_t *bsock = calloc(1, sizeof(NU_Bound_Socket_t));
	if(!bsock){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	return bsock;
}

static NU_Bound_Socket_t *bsock_reuse(NU_Bound_Socket_t **sockets, size_t size, size_t queue_size, unsigned int port, const char *ip_addr){
	size_t i = 0;
	for(;i < size; i++){
		NU_Bound_Socket_t *bsock = sockets[i];
		if(!bsock->is_bound){
			if(!bsock_setup(bsock, queue_size, port, ip_addr)){
				MU_LOG_WARNING(logger, "bsock_setup: 'Was unable to setup bound socket!'");
				return NULL;
			}
			return bsock;
		}
	}
	return NULL;
}

static int bsock_destroy(NU_Bound_Socket_t *bsock){
	if(!bsock) return 0;
	int close_failed = 0;
	if(bsock->is_bound){
		MU_TEMP_FAILURE_RETRY(close_failed, close(bsock->sockfd));
		if(close_failed){
			MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
		}
	}
	free(bsock);
	MU_LOG_VERBOSE(logger, "Destroyed a bound socket!");
	return close_failed == 0;
}

static bool bsock_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	if(!bsock) return false;
	unsigned int port = bsock->port;
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		NU_Connection_t *conn = server->connections[i];
		if(NU_Connection_get_port(conn) == port && NU_Connection_in_use(conn)){
			int is_disconnected = NU_Connection_disconnect(conn);
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
	MU_LOG_INFO(logger, "Unbound from port %u!", port);
	return true;
}

NU_Server_t *NU_Server_create(size_t connection_pool_size, size_t bsock_pool_size, bool synchronized){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	if(!server){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	server->synchronized = synchronized;
	// Keep track of everything allocated. If anything fails to allocate, we will free up all memory allocated in this (very long) block.
	size_t connections_allocated = 0, bsocks_allocated = 0;
	if(synchronized){
		server->lock = malloc(sizeof(pthread_mutex_t));
		if(!server->lock){
			MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		int failure = pthread_mutex_init(server->lock, NULL);
		if(failure){
			MU_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(failure));
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
		NU_Bound_Socket_t *bsock = bsock_create();
		if(!bsock){
			MU_LOG_ERROR(logger, "bsock_create: 'Was unable to create bound socket #%zu'", ++i);
			goto error;
		}
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
		NU_Connection_t *conn = NU_Connection_create(synchronized, logger);
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
				int is_destroyed = NU_Connection_destroy(server->connections[i]);
				if(!is_destroyed){
					MU_LOG_ERROR(logger, "NU_Connection_destroy: 'Was unable to destroy a connection!'");
				}

			}
			free(server->connections);
			}
			if(server->sockets){
				size_t i = 0;
				for(;i < bsocks_allocated; i++){
					int is_destroyed = bsock_destroy(server->sockets[i]);
					if(!is_destroyed){
						MU_LOG_ERROR(logger, "bsock_destroy: 'Was unable to destroy a socket!'");
					}
				}
				free(server->sockets);
			}
			if(synchronized){
				MU_COND_MUTEX_DESTROY(server->lock, logger);
			}
			free(server);
		}
		return NULL;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, size_t queue_size, unsigned int port, const char *ip_addr){
	MU_ARG_CHECK(logger, NULL, server, port > 0, queue_size > 0);
	MU_COND_MUTEX_LOCK(server->lock, logger);
	NU_Bound_Socket_t *bsock = bsock_reuse(server->sockets, server->amount_of_sockets, queue_size, port, ip_addr);
	if(bsock){
		MU_LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr ? ip_addr : "localhost", port);
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		return bsock;
	}
	bsock = bsock_create();
	if(!bsock){
		MU_LOG_ERROR(logger, "bsock_create: 'Was unable to create a bound socket!'");
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		return NULL;
	}
	NU_Bound_Socket_t **tmp_sockets = realloc(server->sockets, sizeof(NU_Bound_Socket_t *) * (server->amount_of_sockets + 1));
	if(!tmp_sockets){
		MU_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		bsock_destroy(bsock);
		return NULL;
	}
	server->sockets = tmp_sockets;
	server->sockets[server->amount_of_sockets++] = bsock;
	bsock_setup(bsock, queue_size, port, ip_addr);
	MU_COND_MUTEX_UNLOCK(server->lock, logger);
	MU_LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr ? ip_addr : "localhost", port);
	return bsock;
}

bool NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	MU_ARG_CHECK(logger, false, server, bsock);
	MU_COND_MUTEX_LOCK(server->lock, logger);
	bsock_unbind(server, bsock);
	MU_COND_MUTEX_UNLOCK(server->lock, logger);
	MU_LOG_INFO(logger, "Unbound from port %u!", bsock->port);
	return true;
}

NU_Connection_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *bsock, long long int timeout){
	MU_ARG_CHECK(logger, NULL, server, bsock);
	char ip_addr[INET_ADDRSTRLEN + 1];
	MU_COND_MUTEX_LOCK(server->lock, logger);
	unsigned int port = bsock->port;
	int bsock_fd = bsock->sockfd;
	bool is_bound = bsock->is_bound;
	MU_COND_MUTEX_UNLOCK(server->lock, logger);
	if(!is_bound){
		MU_LOG_WARNING(logger, "Socket is not bound!");
		return NULL;
	}
	int sockfd = timed_accept(bsock_fd, ip_addr, timeout);
	if(sockfd == -1){
		MU_LOG_VERBOSE(logger, "timed_accept: 'Was unable to accept a connection!'");
		return NULL;
	}
	MU_COND_MUTEX_LOCK(server->lock, logger);
	/// In case that in between acquiring the lock and sockfd the socket gets unbound.
	if(!bsock->is_bound){
		MU_LOG_WARNING(logger, "Socket is not bound!");
		int failure = 0;
		MU_TEMP_FAILURE_RETRY(failure, close(sockfd));
		if(failure){
			MU_LOG_ERROR(logger, "close: '%s'", strerror(errno));
		}
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		return NULL;
	}
	NU_Connection_t *conn = NU_Connection_reuse(server->connections, server->amount_of_connections, sockfd, port, ip_addr, logger);
	if(conn){
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		MU_LOG_INFO(logger, "%s connected to port %d", ip_addr, bsock->port);
		return conn;
	}
	// If the NULL is returned, then a connection could not be reused, hence we initialize a new one below.
	conn = NU_Connection_create(server->synchronized, logger);
	if(!conn){
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		MU_LOG_ERROR(logger, "NU_Connection_create: 'Was unable to create a connection!'");
		return NULL;
	}
	NU_Connection_t **tmp_connections = realloc(server->connections, sizeof(NU_Connection_t *) * (server->amount_of_connections + 1));
	if(!tmp_connections){
		MU_COND_MUTEX_UNLOCK(server->lock, logger);
		MU_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		return NULL;
	}
	server->connections = tmp_connections;
	server->connections[server->amount_of_connections++] = conn;
	int is_initialized = NU_Connection_init(conn, sockfd, bsock->port, ip_addr, logger);
	MU_COND_MUTEX_UNLOCK(server->lock, logger);
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

// TODO: change timeout to long long int and pass NULL instead of &tv if timeout < 0.
NU_Connection_t *NU_Server_accept_any(NU_Server_t *server, long long int timeout){
	MU_ARG_CHECK(logger, NULL, server);
	fd_set are_bound;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO(&are_bound);
 	MU_COND_MUTEX_LOCK(server->lock, logger);
 	size_t i = 0;
 	int max_fd = 0;
 	for(; i < server->amount_of_sockets; i++){
 		NU_Bound_Socket_t *bsock = server->sockets[i];
 		if(!bsock) continue;
 		if(bsock->is_bound){
 			FD_SET(bsock->sockfd, &are_bound);
 			if(bsock->sockfd > max_fd) max_fd = bsock->sockfd;
 		}
 	}
 	MU_COND_MUTEX_UNLOCK(server->lock, logger);
 	int ready = 0; 
 	MU_TEMP_FAILURE_RETRY(ready, select(max_fd + 1, &are_bound, NULL, NULL, timeout < 0 ? NULL : &tv));
 	if(ready <= 0){
 		if(!ready) MU_LOG_VERBOSE(logger, "select: 'Timed out'");
 		else MU_LOG_ERROR(logger, "select: '%s'", strerror(errno));
 		return NULL;
 	}
 	MU_COND_MUTEX_LOCK(server->lock, logger);
 	NU_Bound_Socket_t *bsock = NULL;
 	bool sockfd_found = false;
 	for(i = 0; i < server->amount_of_sockets; i++){
 		bsock = server->sockets[i];
 		if(!bsock) continue;
 		if(bsock->is_bound){
 			if(FD_ISSET(bsock->sockfd, &are_bound)){
 				sockfd_found = true;
 				break;
 			}
 		}
 	}
 	if(!sockfd_found){
 		MU_COND_MUTEX_UNLOCK(server->lock, logger);
 		MU_LOG_VERBOSE(logger, "Could not accept a connection!");
 		return NULL;
 	}
 	MU_COND_MUTEX_UNLOCK(server->lock, logger);
 	return NU_Server_accept(server, bsock, timeout);
}

bool NU_Server_log(NU_Server_t *server, const char *message, ...){
	MU_ARG_CHECK(logger, false, server, message);
	va_list args;
	va_start(args, message);
	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		MU_LOG_WARNING(logger, "vsnprintf: '%s'", strerror(errno));
		return false;
	}
	MU_LOG_SERVER("%s", buffer);
	return true;
}

bool NU_Server_disconnect(NU_Server_t *server, NU_Connection_t *conn){
	MU_ARG_CHECK(logger, false, server, conn);
	int successful = NU_Connection_disconnect(conn);
	if(!successful){
		MU_LOG_ERROR(logger, "NU_Connection_disconnect: 'Was unable to fully disconnect a connection!'");
	}
	MU_LOG_INFO(logger, "Disconnected from %s on port %u!", NU_Connection_get_ip_addr(conn), NU_Connection_get_port(conn));
	return 1;
}

bool NU_Server_shutdown(NU_Server_t *server){
	MU_ARG_CHECK(logger, false, server);
	MU_COND_MUTEX_LOCK(server->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_sockets; i++){
		int successful = bsock_unbind(server, server->sockets[i]);
		if(!successful){
			MU_LOG_ERROR(logger, "bsock_unbind: 'Was unable to fully unbind a socket!'");
		}
	}
	MU_COND_MUTEX_UNLOCK(server->lock, logger);
	return true;
}

bool NU_Server_destroy(NU_Server_t *server){
	MU_ARG_CHECK(logger, false, server);
	int is_shutdown = NU_Server_shutdown(server);
	if(!is_shutdown){
		MU_LOG_ERROR(logger, "NU_Server_shutdown: 'Was unable to shutdown server!'");
		return false;
	}
	MU_COND_MUTEX_LOCK(server->lock, logger);
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		int successful = NU_Connection_destroy(server->connections[i]);
		if(!successful){
			MU_LOG_ERROR(logger, "NU_Connection_destroy: 'Was unable to fully destroy a connection!'");
		}
	}
	for(i = 0;i < server->amount_of_sockets; i++){
		int successful = bsock_destroy(server->sockets[i]);
		if(!successful){
			MU_LOG_ERROR(logger, "bsock_destroy: 'Was unable to fully destroy a bound socket!'");
		}
	}
	MU_COND_MUTEX_UNLOCK(server->lock, logger);
	MU_COND_MUTEX_DESTROY(server->lock, logger);
	free(server->connections);
	free(server->sockets);
	free(server);
	return true;
}
