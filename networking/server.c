#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>

#include "networking/server.h"
#include "misc/signal_retry.h";
#include "misc/argument_check.h";
#include "threading/scoped_lock.h"

struct c_utils_socket {
   /// The bound socket.
   volatile int sockfd;
   /// Port the socket is bound to.
   unsigned int port;
   /// Flag to determine if it is bound.
   volatile bool is_bound;
};


struct c_utils_server {
   /// List of bound sockets owned by this server; I.E How many bound ports.
   struct c_utils_socket **sockets;
   /// List of connections currently connected to.
   struct c_utils_connection **connections;
   /// Size of the list of connections that are connected.
   volatile size_t amount_of_connections;
   /// Size of the list of bound sockets to a port.
   volatile size_t amount_of_sockets;
   /// Lock used for synchronization and thread safety.
   struct c_utils_scoped_lock *lock;
   /// Whether or not to synchronize access.
   bool synchronized;
};

static struct c_utils_logger *logger = NULL;

LOGGER_AUTO_CREATE(logger, "./networking/logs/server.log", "w", LOG_LEVEL_ALL);

/* Server-specific helper functions */

static int timed_accept(int sockfd, char *ip_addr, long long int timeout){
   int accepted = 0;
   fd_set can_accept;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;

   FD_ZERO(&can_accept);
   FD_SET(sockfd, &can_accept);

   C_UTILS_TEMP_FAILURE_RETRY(accepted, select(sockfd + 1, &can_accept, NULL, NULL, timeout < 0 ? NULL : &tv));
   if(accepted <= 0){
      if(!accepted) LOG_VERBOSE(logger, "select: 'Timed out!'");
      else LOG_ERROR(logger, "select: '%s'", strerror(errno));
      return -1;
   }

   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   C_UTILS_TEMP_FAILURE_RETRY(accepted, accept(sockfd, (struct sockaddr *)&addr, &size));
   if(accepted == -1){
      LOG_ERROR(logger, "accept: '%s'", strerror(errno));
      return -1;
   }

   if(ip_addr){
      if(!inet_ntop(AF_INET, &addr, ip_addr , INET_ADDRSTRLEN)) LOG_WARNING(logger, "inet_ntop: '%s'", strerror(errno));
   }

   return accepted;
}

static bool setup_socket(struct c_utils_socket *sock, size_t queue_size, unsigned int port, const char *ip_addr){
	if(!sock) return false;

	int flag = 1;
	struct sockaddr_in my_addr;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		LOG_ERROR(logger, "socket: '%s'", strerror(errno));
		return 0;
	}
	
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
		LOG_ERROR(logger, "setsockopt: '%s'", strerror(errno));
		goto error;
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = ip_addr ? inet_addr(ip_addr) : INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	
	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		LOG_ERROR(logger, "bind: '%s'", strerror(errno));
		goto error;
	}
	
	if(listen(sockfd, queue_size) == -1){
		LOG_ERROR(logger, "listen: '%s'", strerror(errno));
		goto error;
	}
	
	sock->port = port;
	sock->sockfd = sockfd;
	sock->is_bound = 1;
	
	return true;

	error:
		if(sockfd != -1){
			int close_failed;
			C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
			if(close_failed){
				LOG_ERROR(logger, "close: '%s'", strerror(errno));
			}
		}
		return false;
}

static struct c_utils_socket *create_socket(void){
	struct c_utils_socket *sock = calloc(1, sizeof(struct c_utils_socket));
	if(!sock){
		LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	
	return sock;
}

static struct c_utils_socket *reuse_socket(struct c_utils_socket **sockets, size_t size, size_t queue_size, unsigned int port, const char *ip_addr){
	size_t i = 0;
	for(;i < size; i++){
		struct c_utils_socket *sock = sockets[i];
		if(!sock->is_bound){
			if(!setup_socket(sock, queue_size, port, ip_addr)){
				LOG_WARNING(logger, "setup_socket: 'Was unable to setup bound socket!'");
				return NULL;
			}
			return sock;
		}
	}
	return NULL;
}

static int destroy_socket(struct c_utils_socket *sock){
	if(!sock) return 0;
	
	int close_failed = 0;
	if(sock->is_bound){
		C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sock->sockfd));
		if(close_failed){
			LOG_ERROR(logger, "close: '%s'", strerror(errno));
		}
	}
	
	free(sock);
	LOG_VERBOSE(logger, "Destroyed a bound socket!");
	
	return close_failed == 0;
}

static bool unbind_socket(struct c_utils_server *server, struct c_utils_socket *sock){
	if(!sock) return false;
	
	unsigned int port = sock->port;
	
	/*
		When we unbind, we also must disconnect each connection manually. While internally it may
		handle it more elegantly than this (and most likely does), we need to update the connection
		to let it know it's been disconnected and no longer asks like it is still connected to it's
		end-point.
	*/
	size_t i = 0;
	for(;i < server->amount_of_connections; i++){
		struct c_utils_connection *conn = server->connections[i];
		if(c_utils_connection_get_port(conn) == port && c_utils_connection_in_use(conn)){
			int is_disconnected = c_utils_connection_disconnect(conn);
			if(!is_disconnected){
				LOG_ERROR(logger, "c_utils_connection_disconnect: 'Was unable to disconnect a connection!'");
			}
		}
	}
	
	int close_failed;
	C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sock->sockfd));
	if(close_failed){
		LOG_ERROR(logger, "close: '%s'", strerror(errno));
	}

	sock->is_bound = 0;
	LOG_INFO(logger, "Unbound from port %u!", port);
	
	return true;
}

struct c_utils_server *c_utils_server_create(size_t connection_pool_size, size_t sock_pool_size, bool synchronized){
	struct c_utils_server *server = calloc(1, sizeof(struct c_utils_server));
	if(!server){
		LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	
	bool mutex_init = false;
	pthread_mutex_t *lock = NULL;

	if(synchronized){
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

		server->synchronized = true;
	}

	server->lock = SCOPED_LOCK_FROM(lock);
	if(!server->lock) {
		LOG_ERROR(logger, "SCOPED_LOCK_FROM: 'Unable to create scoped lock from mutex!'");
		goto error;
	}

	/*
		Note, that one of huge pains of maintaining a pool of resources is that you must always free them, even if
		other resources fail to be allocated, we must undo what has already been done (and hopefully restore the
		situation to a better state, memory-wise at least). Hence, for each connection and socket, we must both
		keep track of what has been allocated. This is a key definining feature of the goto statement, as
		in an extremely complex construction as this, we can either A) Assert, which means this library is
		useless on low-memory systems, or B) Adapt and give back to the system. Obviously B was chosen.
	*/
	size_t connections_allocated = 0, socks_allocated = 0;
	server->amount_of_sockets = sock_pool_size ? sock_pool_size : 1;

	server->sockets = calloc(server->amount_of_sockets, sizeof(struct c_utils_socket *));
	if(!server->sockets){
		LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}

	size_t i = 0;
	for(;i < server->amount_of_sockets; i++){
		struct c_utils_socket *sock = create_socket();
		if(!sock){
			LOG_ERROR(logger, "create_socket: 'Was unable to create bound socket #%zu'", ++i);
			goto error;
		}
		server->sockets[i] = sock;
		socks_allocated++;
	}
	
	server->amount_of_connections = connection_pool_size ? connection_pool_size : 1;
	server->connections = calloc(server->amount_of_connections, sizeof(struct c_utils_connection *));
	if(!server->connections){
		LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	i = 0;
	for(;i < server->amount_of_connections; i++){
		struct c_utils_connection *conn = c_utils_connection_create(synchronized, logger);
		if(!conn){
			LOG_ASSERT(logger, "c_utils_connection_create: 'Was unable to create connection #%zu!'", ++i);
			goto error;
		}
		server->connections[i] = conn;
		connections_allocated++;
	}
	
	return server;

	/// Deallocate all memory allocated if something were to fail!
	error:
		if(server){
			if(server->lock) {
				c_utils_scoped_lock_destroy(server->lock);
			} else if (lock) {
				if(mutex_init) {
					pthread_mutex_destroy(lock);
				}
				free(lock);
			}
			if(server->connections){
			size_t i = 0;
			for(;i < connections_allocated; i++){
				int is_destroyed = c_utils_connection_destroy(server->connections[i]);
				if(!is_destroyed){
					LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to destroy a connection!'");
				}
			}
			free(server->connections);
			}
			if(server->sockets){
				size_t i = 0;
				for(;i < socks_allocated; i++){
					int is_destroyed = destroy_socket(server->sockets[i]);
					if(!is_destroyed){
						LOG_ERROR(logger, "destroy_socket: 'Was unable to destroy a socket!'");
					}
				}
				free(server->sockets);
			}
			free(server);
		}
		return NULL;
}

struct c_utils_socket *c_utils_server_bind(struct c_utils_server *server, size_t queue_size, unsigned int port, const char *ip_addr){
	ARG_CHECK(logger, NULL, server, port > 0, queue_size > 0);

	struct c_utils_socket *sock;
	SCOPED_LOCK(server->lock) {
		sock = reuse_socket(server->sockets, server->amount_of_sockets, queue_size, port, ip_addr);
		if(sock){
			LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr ? ip_addr : "localhost", port);
			return sock;
		}

		sock = create_socket();
		if(!sock){
			LOG_ERROR(logger, "create_socket: 'Was unable to create a bound socket!'");
			return NULL;
		}

		struct c_utils_socket **tmp_sockets = realloc(server->sockets, sizeof(struct c_utils_socket *) * (server->amount_of_sockets + 1));
		if(!tmp_sockets){
			LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
			destroy_socket(sock);
			return NULL;
		}

		server->sockets = tmp_sockets;
		server->sockets[server->amount_of_sockets++] = sock;

		setup_socket(sock, queue_size, port, ip_addr);
	}
	LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr ? ip_addr : "localhost", port);

	return sock;
}

bool c_utils_server_unbind(struct c_utils_server *server, struct c_utils_socket *sock){
	ARG_CHECK(logger, false, server, sock);

	SCOPED_LOCK(server->lock) unbind_sock(server, sock);
	LOG_INFO(logger, "Unbound from port %u!", sock->port);

	return true;
}

struct c_utils_connection *c_utils_server_accept(struct c_utils_server *server, struct c_utils_socket *sock, long long int timeout){
	ARG_CHECK(logger, NULL, server, sock);

	char ip_addr[INET_ADDRSTRLEN + 1];
	unsigned int port;
	int sock_fd;
	bool is_bound;

	SCOPED_LOCK(server->lock) {
		port = sock->port;
		sock_fd = sock->sockfd;
		is_bound = sock->is_bound;
	}

	if(!is_bound){
		LOG_WARNING(logger, "Socket is not bound!");
		return NULL;
	}

	int sockfd = timed_accept(sock_fd, ip_addr, timeout);
	if(sockfd == -1){
		LOG_VERBOSE(logger, "timed_accept: 'Was unable to accept a connection!'");
		return NULL;
	}

	bool is_initialized;
	struct c_utils_connection *conn = NULL;
	SCOPED_LOCK(server->lock) {
		/// In case that in between acquiring the lock and sockfd the socket gets unbound.
		if(!sock->is_bound){
			LOG_WARNING(logger, "Socket is not bound!");

			int failure = 0;
			C_UTILS_TEMP_FAILURE_RETRY(failure, close(sockfd));
			if(failure){
				LOG_ERROR(logger, "close: '%s'", strerror(errno));
			}

			return NULL;
		}

		conn = c_utils_connection_reuse(server->connections, server->amount_of_connections, sockfd, port, ip_addr, logger);
		if(conn){
			LOG_INFO(logger, "%s connected to port %d", ip_addr, sock->port);
			return conn;
		}

		// If NULL is returned, then a connection could not be reused, hence we initialize a new one below.
		conn = c_utils_connection_create(server->synchronized, logger);
		if(!conn){
			LOG_ERROR(logger, "c_utils_connection_create: 'Was unable to create a connection!'");
			return NULL;
		}

		struct c_utils_connection **tmp_connections = realloc(server->connections, sizeof(struct c_utils_connection *) * (server->amount_of_connections + 1));
		if(!tmp_connections){
			LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
			return NULL;
		}

		server->connections = tmp_connections;
		server->connections[server->amount_of_connections++] = conn;
		
		is_initialized = c_utils_connection_init(conn, sockfd, sock->port, ip_addr, logger);
	}
	if(!is_initialized) {
		LOG_ERROR(logger, "c_utils_connection_init: 'Was unable to initialize a connection!'");
		
		int close_failed;
		C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
		if(close_failed){
			LOG_ERROR(logger, "close: '%s'", strerror(errno));
		}
		
		return NULL;
	}
	LOG_INFO(logger, "%s connected to port %d", ip_addr, sock->port);
	
	return conn;
}

// TODO: change timeout to long long int and pass NULL instead of &tv if timeout < 0.
struct c_utils_connection *c_utils_server_accept_any(struct c_utils_server *server, long long int timeout){
	ARG_CHECK(logger, NULL, server);

	fd_set are_bound;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&are_bound);

    int max_fd = 0;
 	SCOPED_LOCK(server->lock) {
	 	size_t i = 0;
	 	for(; i < server->amount_of_sockets; i++){
	 		struct c_utils_socket *sock = server->sockets[i];
	 		if(!sock) continue;
	 		if(sock->is_bound){
	 			FD_SET(sock->sockfd, &are_bound);
	 			if(sock->sockfd > max_fd) max_fd = sock->sockfd;
	 		}
	 	}
 	}

 	int ready = 0; 
 	C_UTILS_TEMP_FAILURE_RETRY(ready, select(max_fd + 1, &are_bound, NULL, NULL, timeout < 0 ? NULL : &tv));
 	if(ready <= 0){
 		if(!ready) LOG_VERBOSE(logger, "select: 'Timed out'");
 		else LOG_ERROR(logger, "select: '%s'", strerror(errno));
 		return NULL;
 	}

 	struct c_utils_socket *sock = NULL;
 	SCOPED_LOCK(server->lock) {
	 	bool sockfd_found = false;
	 	for(i = 0; i < server->amount_of_sockets; i++){
	 		sock = server->sockets[i];
	 		if(!sock) continue;
	 		if(sock->is_bound){
	 			if(FD_ISSET(sock->sockfd, &are_bound)){
	 				sockfd_found = true;
	 				break;
	 			}
	 		}
	 	}
	 	if(!sockfd_found){
	 		LOG_VERBOSE(logger, "Could not accept a connection!");
	 		return NULL;
	 	}
 	}
 	return c_utils_server_accept(server, sock, timeout);
}

bool c_utils_server_log(struct c_utils_server *server, const char *message, ...){
	ARG_CHECK(logger, false, server, message);

	va_list args;
	va_start(args, message);

	const int buf_size = 1024;
	char buffer[buf_size];
	if(vsnprintf(buffer, buf_size, message, args) < 0){ 
		LOG_WARNING(logger, "vsnprintf: '%s'", strerror(errno));
		return false;
	}

	LOG_CUSTOM(logger, "SERVER", "%s", buffer);
	return true;
}

bool c_utils_server_disconnect(struct c_utils_server *server, struct c_utils_connection *conn){
	ARG_CHECK(logger, false, server, conn);

	int successful = c_utils_connection_disconnect(conn);
	if(!successful){
		LOG_ERROR(logger, "c_utils_connection_disconnect: 'Was unable to fully disconnect a connection!'");
	}

	LOG_INFO(logger, "Disconnected from %s on port %u!", c_utils_connection_get_ip_addr(conn), c_utils_connection_get_port(conn));
	return true;
}

bool c_utils_server_shutdown(struct c_utils_server *server){
	ARG_CHECK(logger, false, server);

	SCOPED_LOCK(server->lock) {
		size_t i = 0;
		for(;i < server->amount_of_sockets; i++){
			int successful = unbind_sock(server, server->sockets[i]);
			if(!successful){
				LOG_ERROR(logger, "unbind_sock: 'Was unable to fully unbind a socket!'");
			}
		}
	}

	return true;
}

bool c_utils_server_destroy(struct c_utils_server *server){
	ARG_CHECK(logger, false, server);

	int is_shutdown = c_utils_server_shutdown(server);
	if(!is_shutdown){
		LOG_ERROR(logger, "c_utils_server_shutdown: 'Was unable to shutdown server!'");
		return false;
	}

	SCOPED_LOCK(server->lock) {
		size_t i = 0;
		for(;i < server->amount_of_connections; i++){
			int successful = c_utils_connection_destroy(server->connections[i]);
			if(!successful){
				LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to fully destroy a connection!'");
			}
		}

		for(i = 0;i < server->amount_of_sockets; i++){
			int successful = destroy_socket(server->sockets[i]);
			if(!successful){
				LOG_ERROR(logger, "destroy_socket: 'Was unable to fully destroy a bound socket!'");
			}
		}
	}

	c_utils_scoped_lock_destroy(server->lock);
	free(server->connections);
	free(server->sockets);
	free(server);

	return true;
}
