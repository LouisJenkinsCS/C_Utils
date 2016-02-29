#ifndef NET_UTILS_SERVER_H
#define NET_UTILS_SERVER_H

#include "../networking/connection.h"

/**
 *  c_utils_server is a simple manager of c_utils_connection objects and c_utils_socket objects in an
 *  efficient manner, with optional synchronized access for thread-safety. The c_utils_socket is
 *  as it's name suggests, a bound socket which you accept connections on. It allows you to do the following...
 *
 *  * Setup and accept connections on an End Point.
 *  * Efficiently Recycle connections to End Points and bound sockets.
 *  * Logs any errors, warnings and information to a log file.
 *  * Cleanly destroy and shutdown existing connections and bound sockets.
 *  * Accept connections on any bound ports.
 *
 *  Note: You should never, ever, EVER free any objects returned by any c_utils_server instance.
 */
struct c_utils_server;

/**
 * @brief Wraps a bound socket on a given port.
 * 
 * This type keeps track of te sockfd that was created from socket() and bind(), and is used
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
struct c_utils_socket;


#ifdef C_UTILS_USE_POSIX_STD
typedef struct c_utils_server server_t;
typedef struct c_utils_socket socket_t;
#define server_create(...) c_utils_server_create(__VA_ARGS__)
#define server_bind(...) c_utils_server_bind(__VA_ARGS__)
#define server_unbind(...) c_utils_server_unbind(__VA_ARGS__)
#define server_accept(...) c_utils_server_accept(__VA_ARGS__)
#define server_shutdown(...) c_utils_server_shutdown(__VA_ARGS__)
#define server_disconnect(...) c_utils_server_disconnect(__VA_ARGS__)
#define server_log(...) c_utils_server_log(__VA_ARGS__)
#define server_accept(...) c_utils_server_accept(__VA_ARGS__)
#define server_accept_any(...) c_utils_server_accept_any(__VA_ARGS__)
#define server_destroy(...) c_utils_server_destroy(__VA_ARGS__)
#endif

/**
 * Create and initialize a c_utils_server instance, creating a pool of connection_pool_size connections, and
 * sock_pool_size bound sockets, with optional synchronized access for thread-safety.
 * @param connection_pool_size Initial pool size of connections, minimum of 1.
 * @param sock_pool_size Initiali pool size of bound sockets, minimum of 1.
 * @param synchronized if true, all access is sycnhronized.
 * @return A fully initialized instance of c_utils_server, or NULL on error.
 */
struct c_utils_server *c_utils_server_create(size_t connection_pool_size, size_t sock_pool_size, bool synchronized);

/**
 * Binds the instance to given port, returning a c_utils_socket instance managed by the c_utils_server, which
 * should not be freed. 
 * @param server Instance of server.
 * @param queue_size Maximum amount of connections in backlog.
 * @param port Port to bind to.
 * @param ip_addr IP address to bind to.
 * @return A bound socket, or NULL on error.
 */
struct c_utils_socket *c_utils_server_bind(struct c_utils_server *server, size_t queue_size, unsigned int port, const char *ip_addr);

/**
 * Unbinds the instance from the given bound socket, disconnecting any connections using the bound socket, hence may block
 * until the connections are finished communicating.
 * @param server Instance of server.
 * @param socket Bound Socket to unbind from.
 * @return true on success, false on error.
 */
bool c_utils_server_unbind(struct c_utils_server *server, struct c_utils_socket *socket);

/**
 * Accepts a new connection on a specified bound socket, blocking up to timeout or until a connection has been
 * successfully established.
 * @param server Instance of server.
 * @param socket Bound socket to accept on.
 * @param timeout Maximum timeout to block for.
 * @return A connection connected to an end-point, or NULL on error.
 */
struct c_utils_connection *c_utils_server_accept(struct c_utils_server *server, struct c_utils_socket *socket, long long int timeout);

/**
 * Accepts a new connection on ANY bound socket, meaning the ones bound to the server instance, up to the timeout
 * or until a connection has been successfully established.
 * @param server Instance of server.
 * @param timeout Maximum timeout to block for.
 * @return A connection connected to an end-point, or NULL on error.
 */
struct c_utils_connection *c_utils_server_accept_any(struct c_utils_server *server, long long int timeout);

/**
 * Shutdowns the server, unbinds any ports, and disconnects any connections connected to those ports.
 * @param server Instance of server.
 * @return True on success, false on error.
 */
bool c_utils_server_shutdown(struct c_utils_server *server);

/**
 * Disconnects from the connection, marking it as available for recycle to the server.
 * @param server Instance of server.
 * @param conn Connection to disconnect from.
 * @return True on success, false on error.
 */
bool c_utils_server_disconnect(struct c_utils_server *server, struct c_utils_connection *conn);

/**
 * Logs the passed message and variadic arguments to the server's log-files.
 * @param server Instance of server.
 * @param message Message to log.
 * @param ... Variadic arguments to pass to the format message.
 * @return true on success, false on error.
 */
bool c_utils_server_log(struct c_utils_server *server, const char *message, ...);

/**
 * Destroys and shutsdown the server.
 * @param server Instance of server.
 * @return true on success, false on error.
 */
bool c_utils_server_destroy(struct c_utils_server *server);

#endif /* endif NET_UTILS_SERVER_H */
