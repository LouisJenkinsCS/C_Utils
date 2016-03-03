#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../networking/server.h"
#include "../misc/signal_retry.h"
#include "../misc/argument_check.h"
#include "../misc/alloc_check.h"
#include "../threading/scoped_lock.h"

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
   struct c_utils_socket **sock_pool;
   /// List of connections currently connected to.
   struct c_utils_connection **conn_pool;
   /// Size of the list of connections that are connected.
   volatile size_t conn_pool_size;
   /// Size of the list of bound sock_pool to a port.
   volatile size_t sock_pool_size;
   /// Lock used for synchronization and thread safety.
   struct c_utils_scoped_lock *lock;
   /// Whether or not to synchronize access.
   bool synchronized;
};

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./networking/logs/server.log", "w", C_UTILS_LOG_LEVEL_ALL);

/* Server-specific helper functions */

static int timed_accept(int sockfd, char *ip_addr, long long int timeout) {
   int accepted = 0;
   fd_set can_accept;
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;

   FD_ZERO(&can_accept);
   FD_SET(sockfd, &can_accept);

   C_UTILS_TEMP_FAILURE_RETRY(accepted, select(sockfd + 1, &can_accept, NULL, NULL, timeout < 0 ? NULL : &tv));
   if (accepted <= 0) {
      if (!accepted) C_UTILS_LOG_VERBOSE(logger, "select: 'Timed out!'");
      else C_UTILS_LOG_ERROR(logger, "select: '%s'", strerror(errno));
      return -1;
   }

   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   C_UTILS_TEMP_FAILURE_RETRY(accepted, accept(sockfd, (struct sockaddr *)&addr, &size));
   if (accepted == -1) {
      C_UTILS_LOG_ERROR(logger, "accept: '%s'", strerror(errno));
      return -1;
   }

   if (ip_addr)  
      if (!inet_ntop(AF_INET, &addr, ip_addr , INET_ADDRSTRLEN)) C_UTILS_LOG_WARNING(logger, "inet_ntop: '%s'", strerror(errno));
   

   return accepted;
}

static bool setup_socket(struct c_utils_socket *sock, size_t queue_size, unsigned int port, const char *ip_addr) {
	if (!sock) return false;

	int flag = 1;
	struct sockaddr_in my_addr;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		C_UTILS_LOG_ERROR(logger, "socket: '%s'", strerror(errno));
		return 0;
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
		C_UTILS_LOG_ERROR(logger, "setsockopt: '%s'", strerror(errno));
		goto error;
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = ip_addr ? inet_addr(ip_addr) : INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		C_UTILS_LOG_ERROR(logger, "bind: '%s'", strerror(errno));
		goto error;
	}
	
	if (listen(sockfd, queue_size) == -1) {
		C_UTILS_LOG_ERROR(logger, "listen: '%s'", strerror(errno));
		goto error;
	}
	
	sock->port = port;
	sock->sockfd = sockfd;
	sock->is_bound = 1;
	
	return true;

	error:
		if (sockfd != -1) {
			int close_failed;
			C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
			if (close_failed)  
				C_UTILS_LOG_ERROR(logger, "close: '%s'", strerror(errno));
			
		}
		return false;
}

static struct c_utils_socket *reuse_socket(struct c_utils_socket **sock_pool, size_t size, size_t queue_size, unsigned int port, const char *ip_addr) {
	size_t i = 0;
	for (;i < size; i++) {
		struct c_utils_socket *sock = sock_pool[i];
		if (!sock->is_bound) {
			if (!setup_socket(sock, queue_size, port, ip_addr)) {
				C_UTILS_LOG_WARNING(logger, "setup_socket: 'Was unable to setup bound socket!'");
				return NULL;
			}
			return sock;
		}
	}
	return NULL;
}

static int destroy_socket(struct c_utils_socket *sock) {
	if (!sock) return 0;
	
	int close_failed = 0;
	if (sock->is_bound) {
		C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sock->sockfd));
		if (close_failed)  
			C_UTILS_LOG_ERROR(logger, "close: '%s'", strerror(errno));
		
	}
	
	free(sock);
	C_UTILS_LOG_VERBOSE(logger, "Destroyed a bound socket!");
	
	return close_failed == 0;
}

static bool unbind_socket(struct c_utils_server *server, struct c_utils_socket *sock) {
	if (!sock) 
		return false;
	
	unsigned int port = sock->port;
	
	/*
		When we unbind, we also must disconnect each connection manually. While internally it may
		handle it more elegantly than this (and most likely does), we need to update the connection
		to let it know it's been disconnected and no longer asks like it is still connected to it's
		end-point.
	*/
	for (size_t i = 0; i < server->conn_pool_size; i++) {
		struct c_utils_connection *conn = server->conn_pool[i];
		if (c_utils_connection_get_port(conn) == port && c_utils_connection_in_use(conn))
			if (!c_utils_connection_disconnect(conn))
				C_UTILS_LOG_ERROR(logger, "c_utils_connection_disconnect: 'Was unable to disconnect a connection!'");
	}
	
	int close_failed;
	C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sock->sockfd));
	if (close_failed)  
		C_UTILS_LOG_ERROR(logger, "close: '%s'", strerror(errno));
	

	sock->is_bound = 0;
	C_UTILS_LOG_INFO(logger, "Unbound from port %u!", port);
	
	return true;
}

struct c_utils_server *c_utils_server_create(size_t connection_pool_size, size_t sock_pool_size, bool synchronized) {
	struct c_utils_server *server;
	C_UTILS_ON_BAD_CALLOC(server, logger, sizeof(*server))
		goto err;

	server->lock = synchronized ? c_utils_scoped_lock_mutex(NULL, logger) : c_utils_scoped_lock_no_op();
	if (!server->lock) {
		C_UTILS_LOG_ERROR(logger, "Was unable to create scoped_lock!");
		goto err_lock;
	}

	/*
		Note, that one of huge pains of maintaining a pool of resources is that you must always free them, even if
		other resources fail to be allocated, we must undo what has already been done (and hopefully restore the
		situation to a better state, memory-wise at least). Hence, for each connection and socket, we must both
		keep track of what has been allocated. This is a key definining feature of the goto statement, as
		in an extremely complex construction as this, we can either A) Assert, which means this library is
		useless on low-memory systems, or B) Adapt and give back to the system. Obviously B was chosen.
	*/

	size_t socks_allocated = 0;
	server->sock_pool_size = sock_pool_size ? sock_pool_size : 1;
	C_UTILS_ON_BAD_CALLOC(server->sock_pool, logger, sizeof(*server->sock_pool))
		goto err_sock_pool;

	for (size_t i = 0; i < server->sock_pool_size; i++) {
		struct c_utils_socket *sock;
		C_UTILS_ON_BAD_CALLOC(sock, logger, sizeof(*sock))
			goto err_sock_alloc;

		server->sock_pool[i] = sock;
		socks_allocated++;
	}

	size_t conns_allocated = 0;
	server->conn_pool_size = connection_pool_size ? connection_pool_size : 1;
	C_UTILS_ON_BAD_CALLOC(server->conn_pool, logger, sizeof(*server->conn_pool))
		goto err_conn_pool;

	for (size_t i = 0; i < server->conn_pool_size; i++) {
		struct c_utils_connection *conn = c_utils_connection_create(synchronized, logger);
		if (!conn) {
			C_UTILS_LOG_ERROR(logger, "c_utils_connection_create: 'Was unable to create connection #%zu!'", ++i);
			goto err_conn_alloc;
		}

		server->conn_pool[i] = conn;
		conns_allocated++;
	}

	return server;

	err_conn_alloc:
		for(size_t i = 0; i < conns_allocated; i++)
			c_utils_connection_destroy(server->conn_pool[i]);

		free(server->conn_pool);
	err_conn_pool:
	err_sock_alloc:
		for(size_t i = 0; i < socks_allocated; i++)
			free(socket);

		free(server->sock_pool);
	err_sock_pool:
		c_utils_scoped_lock_destroy(server->lock);
	err_lock:
		free(server);
	err:
		return NULL;
}

struct c_utils_socket *c_utils_server_bind(struct c_utils_server *server, size_t queue_size, unsigned int port, const char *ip_addr) {
	C_UTILS_ARG_CHECK(logger, NULL, server, port > 0, queue_size > 0);

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(server->lock) {
		struct c_utils_socket *sock = reuse_socket(server->sock_pool, server->sock_pool_size, queue_size, port, ip_addr);
		if (sock) {
			C_UTILS_LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr ? ip_addr : "localhost", port);
			return sock;
		}

		C_UTILS_ON_BAD_CALLOC(sock, logger, sizeof(*sock))
			return NULL;

		C_UTILS_ON_BAD_REALLOC(&server->sock_pool, logger, sizeof(struct c_utils_socket *) * (server->sock_pool_size + 1)) {
			free(sock);
			return NULL;
		}

		server->sock_pool[server->sock_pool_size++] = sock;

		setup_socket(sock, queue_size, port, ip_addr);
		C_UTILS_LOG_INFO(logger, "Bound a socket to %s on port %u!", ip_addr ? ip_addr : "localhost", port);

		return sock;
	} // Release Mutex

	C_UTILS_UNACCESSIBLE;
}

bool c_utils_server_unbind(struct c_utils_server *server, struct c_utils_socket *sock) {
	C_UTILS_ARG_CHECK(logger, false, server, sock);

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(server->lock) unbind_socket(server, sock);
	C_UTILS_LOG_INFO(logger, "Unbound from port %u!", sock->port);

	return true;
}

struct c_utils_connection *c_utils_server_accept(struct c_utils_server *server, struct c_utils_socket *sock, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, NULL, server, sock);

	char ip_addr[INET_ADDRSTRLEN + 1];
	unsigned int port;
	int sock_fd;
	bool is_bound;

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(server->lock) {
		port = sock->port;
		sock_fd = sock->sockfd;
		is_bound = sock->is_bound;
	} // Release Mutex

	if (!is_bound) {
		C_UTILS_LOG_WARNING(logger, "Socket is not bound!");
		return NULL;
	}

	int sockfd = timed_accept(sock_fd, ip_addr, timeout);
	if (sockfd == -1) {
		C_UTILS_LOG_VERBOSE(logger, "timed_accept: 'Was unable to accept a connection!'");
		return NULL;
	}

	bool is_initialized;
	struct c_utils_connection *conn = NULL;

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(server->lock) {
		/// In case that in between acquiring the lock and sockfd the socket gets unbound.
		if (!sock->is_bound) {
			C_UTILS_LOG_WARNING(logger, "Socket is not bound!");

			int failure = 0;
			C_UTILS_TEMP_FAILURE_RETRY(failure, close(sockfd));
			if (failure)  
				C_UTILS_LOG_ERROR(logger, "close: '%s'", strerror(errno));
			

			return NULL;
		}

		conn = c_utils_connection_reuse(server->conn_pool, server->conn_pool_size, sockfd, port, ip_addr, logger);
		if (conn) {
			C_UTILS_LOG_INFO(logger, "%s connected to port %d", ip_addr, sock->port);
			return conn;
		}

		// If NULL is returned, then a connection could not be reused, hence we initialize a new one below.
		conn = c_utils_connection_create(server->synchronized, logger);
		if (!conn) {
			C_UTILS_LOG_ERROR(logger, "c_utils_connection_create: 'Was unable to create a connection!'");
			return NULL;
		}

		C_UTILS_ON_BAD_REALLOC(&server->conn_pool, logger, (sizeof(struct c_utils_connection *) * (server->conn_pool_size + 1)))
			return NULL;
		
		server->conn_pool[server->conn_pool_size++] = conn;
		
		is_initialized = c_utils_connection_init(conn, sockfd, sock->port, ip_addr, logger);
	} // Release Mutex

	if (!is_initialized) {
		C_UTILS_LOG_ERROR(logger, "c_utils_connection_init: 'Was unable to initialize a connection!'");
		
		int close_failed;
		C_UTILS_TEMP_FAILURE_RETRY(close_failed, close(sockfd));
		if (close_failed)  
			C_UTILS_LOG_ERROR(logger, "close: '%s'", strerror(errno));
		
		
		return NULL;
	}
	C_UTILS_LOG_INFO(logger, "%s connected to port %d", ip_addr, sock->port);
	
	return conn;
}

// TODO: change timeout to long long int and pass NULL instead of &tv if timeout < 0.
struct c_utils_connection *c_utils_server_accept_any(struct c_utils_server *server, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, NULL, server);

	fd_set are_bound;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&are_bound);

    int max_fd = 0;

   	// Acquire Mutex
 	C_UTILS_SCOPED_LOCK(server->lock) {
	 	size_t i = 0;
	 	for (; i < server->sock_pool_size; i++) {
	 		struct c_utils_socket *sock = server->sock_pool[i];
	 		if (!sock)
	 			continue;

	 		if (sock->is_bound) {
	 			FD_SET(sock->sockfd, &are_bound);
	 			
	 			if (sock->sockfd > max_fd) 
	 				max_fd = sock->sockfd;
	 		}
	 	}
 	} // Release Mutex

 	int ready = 0; 
 	C_UTILS_TEMP_FAILURE_RETRY(ready, select(max_fd + 1, &are_bound, NULL, NULL, timeout < 0 ? NULL : &tv));
 	if (ready <= 0) {
 		if (!ready) 
 			C_UTILS_LOG_VERBOSE(logger, "select: 'Timed out'");
 		else 
 			C_UTILS_LOG_ERROR(logger, "select: '%s'", strerror(errno));
 		return NULL;
 	}

 	struct c_utils_socket *sock = NULL;

 	// Acquire Mutex
 	C_UTILS_SCOPED_LOCK(server->lock) {
	 	bool sockfd_found = false;
	 	for (size_t i = 0; i < server->sock_pool_size; i++) {
	 		sock = server->sock_pool[i];
	 		if (!sock) 
	 			continue;
	 		if (sock->is_bound) {
	 			if (FD_ISSET(sock->sockfd, &are_bound)) {
	 				sockfd_found = true;
	 				break;
	 			}
	 		}
	 	}
	 	if (!sockfd_found) {
	 		C_UTILS_LOG_VERBOSE(logger, "Could not accept a connection!");
	 		return NULL;
	 	}
 	} // Release Mutex
 	return c_utils_server_accept(server, sock, timeout);
}

bool c_utils_server_log(struct c_utils_server *server, const char *message, ...) {
	C_UTILS_ARG_CHECK(logger, false, server, message);

	va_list args;
	va_start(args, message);

	const int buf_size = 1024;
	char buffer[buf_size];
	if (vsnprintf(buffer, buf_size, message, args) < 0) { 
		C_UTILS_LOG_WARNING(logger, "vsnprintf: '%s'", strerror(errno));
		return false;
	}

	C_UTILS_LOG_CUSTOM(logger, "SERVER", "%s", buffer);
	return true;
}

bool c_utils_server_disconnect(struct c_utils_server *server, struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(logger, false, server, conn);

	if (!c_utils_connection_disconnect(conn))  
		C_UTILS_LOG_ERROR(logger, "c_utils_connection_disconnect: 'Was unable to fully disconnect a connection!'");
	
	C_UTILS_LOG_INFO(logger, "Disconnected from %s on port %u!", c_utils_connection_get_ip_addr(conn), c_utils_connection_get_port(conn));
	return true;
}

bool c_utils_server_shutdown(struct c_utils_server *server) {
	C_UTILS_ARG_CHECK(logger, false, server);

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(server->lock)
		for (size_t i = 0; i < server->sock_pool_size; i++)
			if (!unbind_socket(server, server->sock_pool[i]))
				C_UTILS_LOG_ERROR(logger, "unbind_socket: 'Was unable to fully unbind a socket!'");

	return true;
}

bool c_utils_server_destroy(struct c_utils_server *server) {
	C_UTILS_ARG_CHECK(logger, false, server);

	if (!c_utils_server_shutdown(server)) {
		C_UTILS_LOG_ERROR(logger, "c_utils_server_shutdown: 'Was unable to shutdown server!'");
		return false;
	}

	// Acquire Mutex
	C_UTILS_SCOPED_LOCK(server->lock) {
		for (size_t i = 0; i < server->conn_pool_size; i++)
			if (!c_utils_connection_destroy(server->conn_pool[i]))
				C_UTILS_LOG_ERROR(logger, "c_utils_connection_destroy: 'Was unable to fully destroy a connection!'");

		for (size_t i = 0; i < server->sock_pool_size; i++)
			if (!destroy_socket(server->sock_pool[i]))
				C_UTILS_LOG_ERROR(logger, "destroy_socket: 'Was unable to fully destroy a bound socket!'");
	} // Release Mutex

	c_utils_scoped_lock_destroy(server->lock);
	free(server->conn_pool);
	free(server->sock_pool);
	free(server);

	return true;
}
