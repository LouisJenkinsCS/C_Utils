#ifndef NET_UTILS_SERVER_H
#define NET_UTILS_SERVER_H

#include <NU_Connection.h>

<<<<<<< HEAD
/**
 * @brief Wraps a bound socket on a given port.
 * 
 * This type keeps track of the sockfd that was created from socket() and bind(), and is used
=======
/*
   NU_Server_t is a simple manager of NU_Connection_t instances and NU_Bound_Socket_t instances in an
   efficient manner, with optional synchronized access for thread-safety. The NU_Bound_Socket_t is
   as it's name suggests, a bound socket which you accept connections on. It allows you to do the following...

   * Setup and accept connections on an End Point.
   * Efficiently Recycle connections to End Points and bound sockets.
   * Logs any errors, warnings and information to a log file.
   * Cleanly destroy and shutdown existing connections and bound sockets.
   * Accept connections on any bound ports.

   Note: You should never, ever, EVER free any instances returned by any NU_Server_t instance.
*/

/**
 * @brief Wraps a bound socket on a given port.
 * 
 * This type keeps track of te sockfd that was created from socket() and bind(), and is used
>>>>>>> development
 * when attempting to accept a new conn on the socket encapsulated within. It keeps track of
 * the port it is bound to, as well as function as a list for multiple bound socket. 
 * 
 * This type is reusable, and is good for if you wish to create more than one socket, as it helps
 * make managing them easier. Also, if you happen to unbind or shutdown a server without destroying it,
 * as an example, and wish to rebind it to new ports, these types will be recycled, minimizing the overhead
 * of creation.
 * 
 * This type must NOT be freed, as it will cause undefined behavior; instead, it is freed when the server is destroyed.
 */
typedef struct NU_Bound_Socket_t{
   /// The bound socket.
   volatile int sockfd;
   /// Port the socket is bound to.
   unsigned int port;
   /// Flag to determine if it is bound.
   volatile bool is_bound;
} NU_Bound_Socket_t;

/**
 * A manager for bound sockets and connections, recycles and re-uses them when accepting
 * and binding new connection/sockets. 
 */
typedef struct {
   /// List of bound sockets owned by this server; I.E How many bound ports.
   NU_Bound_Socket_t **sockets;
   /// List of connections currently connected to.
   NU_Connection_t **connections;
   /// Size of the list of connections that are connected.
   volatile size_t amount_of_connections;
   /// Size of the list of bound sockets to a port.
   volatile size_t amount_of_sockets;
   /// Lock used for synchronization and thread safety.
   pthread_mutex_t *lock;
   /// Whether or not to synchronize access.
   bool synchronized;
} NU_Server_t;

/**
<<<<<<< HEAD
 * 
 * @param connection_pool_size
 * @param bsock_pool_size
 * @param synchronized
 * @return 
=======
 * Create and initialize a NU_Server_t instance, creating a pool of connection_pool_size connections, and
 * bsock_pool_size bound sockets, with optional synchronized access for thread-safety.
 * @param connection_pool_size Initial pool size of connections, minimum of 1.
 * @param bsock_pool_size Initiali pool size of bound sockets, minimum of 1.
 * @param synchronized if true, all access is sycnhronized.
 * @return A fully initialized instance of NU_Server_t, or NULL on error.
>>>>>>> development
 */
NU_Server_t *NU_Server_create(size_t connection_pool_size, size_t bsock_pool_size, bool synchronized);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @param queue_size
 * @param port
 * @param ip_addr
 * @return 
=======
 * Binds the instance to given port, returning a NU_Bound_Socket_t instance managed by the NU_Server_t, which
 * should not be freed. 
 * @param server Instance of server.
 * @param queue_size Maximum amount of connections in backlog.
 * @param port Port to bind to.
 * @param ip_addr IP address to bind to.
 * @return A bound socket, or NULL on error.
>>>>>>> development
 */
NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, size_t queue_size, unsigned int port, const char *ip_addr);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @param socket
 * @return 
=======
 * Unbinds the instance from the given bound socket, disconnecting any connections using the bound socket, hence may block
 * until the connections are finished communicating.
 * @param server Instance of server.
 * @param socket Bound Socket to unbind from.
 * @return true on success, false on error.
>>>>>>> development
 */
bool NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *socket);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @param socket
 * @param timeout
 * @return 
=======
 * Accepts a new connection on a specified bound socket, blocking up to timeout or until a connection has been
 * successfully established.
 * @param server Instance of server.
 * @param socket Bound socket to accept on.
 * @param timeout Maximum timeout to block for.
 * @return A NU_Connection_t instance connected to an end-point, or NULL on error.
>>>>>>> development
 */
NU_Connection_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *socket, long long int timeout);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @param timeout
 * @return 
=======
 * Accepts a new connection on ANY bound socket, meaning the ones bound to the server instance, up to the timeout
 * or until a connection has been successfully established.
 * @param server Instance of server.
 * @param timeout Maximum timeout to block for.
 * @return A NU_Connection_t instance connected to an end-point, or NULL on error.
>>>>>>> development
 */
NU_Connection_t *NU_Server_accept_any(NU_Server_t *server, long long int timeout);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @return 
=======
 * Shutdowns the server, unbinds any ports, and disconnects any connections connected to those ports.
 * @param server Instance of server.
 * @return True on success, false on error.
>>>>>>> development
 */
bool NU_Server_shutdown(NU_Server_t *server);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @param conn
 * @return 
=======
 * Disconnects from the connection, marking it as available for recycle to the server.
 * @param server Instance of server.
 * @param conn Connection to disconnect from.
 * @return True on success, false on error.
>>>>>>> development
 */
bool NU_Server_disconnect(NU_Server_t *server, NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @param message
 * @param ...
 * @return 
=======
 * Logs the passed message and variadic arguments to the server's log-files.
 * @param server Instance of server.
 * @param message Message to log.
 * @param ... Variadic arguments to pass to the format message.
 * @return true on success, false on error.
>>>>>>> development
 */
bool NU_Server_log(NU_Server_t *server, const char *message, ...);

/**
<<<<<<< HEAD
 * 
 * @param server
 * @return 
=======
 * Destroys and shutsdown the server.
 * @param server Instance of server.
 * @return true on success, false on error.
>>>>>>> development
 */
bool NU_Server_destroy(NU_Server_t *server);

#endif /* endif NET_UTILS_SERVER_H */
