#ifndef NET_UTILS_SERVER_H
#define NET_UTILS_SERVER_H

#include <NU_Connection.h>

/**
 * @brief Wraps a bound socket on a given port.
 * 
 * This type keeps track of the sockfd that was created from socket() and bind(), and is used
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
 * 
 * @param connection_pool_size
 * @param bsock_pool_size
 * @param synchronized
 * @return 
 */
NU_Server_t *NU_Server_create(size_t connection_pool_size, size_t bsock_pool_size, bool synchronized);

/**
 * 
 * @param server
 * @param queue_size
 * @param port
 * @param ip_addr
 * @return 
 */
NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, size_t queue_size, unsigned int port, const char *ip_addr);

/**
 * 
 * @param server
 * @param socket
 * @return 
 */
bool NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *socket);

/**
 * 
 * @param server
 * @param socket
 * @param timeout
 * @return 
 */
NU_Connection_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *socket, long long int timeout);

/**
 * 
 * @param server
 * @param timeout
 * @return 
 */
NU_Connection_t *NU_Server_accept_any(NU_Server_t *server, long long int timeout);

/**
 * 
 * @param server
 * @return 
 */
bool NU_Server_shutdown(NU_Server_t *server);

/**
 * 
 * @param server
 * @param conn
 * @return 
 */
bool NU_Server_disconnect(NU_Server_t *server, NU_Connection_t *conn);

/**
 * 
 * @param server
 * @param message
 * @param ...
 * @return 
 */
bool NU_Server_log(NU_Server_t *server, const char *message, ...);

/**
 * 
 * @param server
 * @return 
 */
bool NU_Server_destroy(NU_Server_t *server);

#endif /* endif NET_UTILS_SERVER_H */
