#ifndef NET_UTILS_SERVER_H
#define NET_UTILS_SERVER_H

#include <NU_Helper.h>
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
   /// RWLock to ensure thread safety.
   pthread_rwlock_t *lock;
   /// Flag to determine if it is bound.
   volatile unsigned char is_bound;
} NU_Bound_Socket_t;

/**
 * @brief Wraps a server's created, bound and listening socket as well keeping track of connected connections.
 * 
 * This type is reusable, in the sense that not only can you bind more than one socket, but also that it's
 * bound sockets are reusable, as well as it's connections. This type allows the user to create a robust server
 * which allows it to easily keep track of more than one bound socket, and connections connected, as well as 
 * the total data sent and received from/to this server. 
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
   /// RWLock associated with server for type safety.
   pthread_rwlock_t *lock;
   /// Keep track of overall data-usage.
   NU_Atomic_Data_t *data;
} NU_Server_t;

/* Create a fully initialized server that is unconnected. The socket used is
   bound to the passed port, but no connections are being accepted on creation. */
NU_Server_t *NU_Server_create(size_t initial_connections, unsigned char init_locks);

/* Bind the server to a port. Can be used multiple times, meaning the server can be bound to more
   than one port. The amount specified will be the amount to listen for. */
NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, const char *ip_addr, unsigned int port, size_t amount, unsigned char init_locks);

/* Will unbind the server from the port specified in socket. Will free the socket! */
int NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *socket);

/* Accept new connections until the timeout ellapses, up to the given amount. The returned
   connections should not be freed, and it is also managed by the server. */
NU_Connection_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *socket,  unsigned int timeout);

/* Send data to the requested client. */
size_t NU_Server_send(NU_Server_t *server, NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout);

/* Receives data from any of current connections. */
size_t NU_Server_receive(NU_Server_t *server, NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout);

/* Receive data from the socket and feed it into a file. sendfile is used for maximum efficiency. */
size_t NU_Server_receive_to_file(NU_Server_t *server, NU_Connection_t *conn, FILE *file, size_t buffer_size, unsigned int timeout);

/* Reads from the file, then sends it to the socket for as long as the timeout. */
size_t NU_Server_send_file(NU_Server_t* server, NU_Connection_t *conn, FILE* file, size_t buffer_size, unsigned int timeout);

/* Blocks for requested timeout or until one of the client sockets passed are available for receiving. */
NU_Connection_t **NU_Server_select_receive(NU_Server_t *server, NU_Connection_t **connections, size_t *size, unsigned int timeout);

/* Blocks for requested timeout or until one of the client sockets passed are available for sending. */
NU_Connection_t **NU_Server_select_send(NU_Server_t *server, NU_Connection_t **connections, size_t *size, unsigned int timeout);

/* Returns a string representation about this client, including but not limited to:
   1) Server's port number binded to as well as local IP.
   2) List of all client's currently connected to, as well as messages and data sent/received from each individually.
   3) Total amount of data and messages sent/received. */
char *NU_Server_about(NU_Server_t *server);

/* The server will no longer be accepting current connections, but will continue dealing with it's
   current connections until the time specified ellapses, upon which it will close all connections. */
int NU_Server_shutdown(NU_Server_t *server);

/* Disconnect the server from the client. */
int NU_Server_disconnect(NU_Server_t *server, NU_Connection_t *conn);

/* Allows the user to log to server's logfile. */
int NU_Server_log(NU_Server_t *server, const char *message, ...);

/* The server will immediately close all connections, free up all resources, and destroy itself. */
int NU_Server_destroy(NU_Server_t *server);

#endif /* endif NET_UTILS_SERVER_H */
