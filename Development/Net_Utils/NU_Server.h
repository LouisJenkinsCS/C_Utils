#ifndef NET_UTILS_SERVER_H
#define NET_UTILS_SERVER_H

#include <NU_Helper.h>

/// Logs server-specific messages to the server's logfile.
#define MU_LOG_SERVER(logger, message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

typedef struct NU_Bound_Socket_t{
   /// The bound socket.
   volatile int sockfd;
   /// Port the socket is bound to.
   unsigned int port;
   /// The next socket the server is listening on.
   struct NU_Bound_Socket_t *next;
} NU_Bound_Socket_t;

typedef struct NU_Client_Socket_t{
   /// Socket file descriptor associated with a client.
   volatile int sockfd;
   /// The IP address the client is connected on.
   char ip_addr[INET_ADDRSTRLEN];
   /// The port this client is connected to!
   unsigned int port;
   /// Buffer that any data will be read into.
   NU_Bounded_Buffer_t *bbuf;
   /// The next client currently connected to.
   struct NU_Client_Socket_t *next;
} NU_Client_Socket_t;

typedef struct {
   /// List of bound sockets owned by this server; I.E How many bound ports.
   NU_Bound_Socket_t *sockets;
   /// List of clients currently connected to.
   NU_Client_Socket_t *clients;
   /// Size of the list of clients that are connected.
   volatile size_t amount_of_clients;
   /// Size of the list of bound sockets to a port.
   volatile size_t amount_of_sockets;
   /// Keep track of overall data-usage.
   NU_Collective_Data_t data;
} NU_Server_t;

/* Create a fully initialized server that is unconnected. The socket used is
   bound to the passed port, but no connections are being accepted on creation. */
NU_Server_t *NU_Server_create();

/* Bind the server to a port. Can be used multiple times, meaning the server can be bound to more
   than one port. The amount specified will be the amount to listen for. */
NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, unsigned int port, size_t amount,  int flags);

/* Will unbind the server from the port specified in socket. Will free the socket! */
int NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *socket, const char *message);

/* Accept new connections until the timeout ellapses, up to the given amount. The returned
   clients should not be freed, and it is also managed by the server. */
NU_Client_Socket_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *socket,  unsigned int timeout);

/* Send data to the requested client. */
size_t NU_Server_send(NU_Server_t *server, NU_Client_Socket_t *client, const char *message, unsigned int timeout);

/* Receives data from any of current connections. */
const char *NU_Server_receive(NU_Server_t *server, NU_Client_Socket_t *client, size_t buffer_size, unsigned int timeout);

/* Receive data from the socket and feed it into a file. sendfile is used for maximum efficiency. */
size_t NU_Server_receive_to_file(NU_Server_t *server, NU_Client_Socket_t *client, FILE *file, size_t buffer_size, unsigned int timeout, int flags);

/* Reads from the file, then sends it to the socket for as long as the timeout. */
size_t NU_Server_send_file(NU_Server_t *server, NU_Client_Socket_t *client, FILE *file, size_t buffer_size, unsigned int timeout);

/* Blocks for requested timeout or until one of the client sockets passed are available for receiving. */
NU_Client_Socket_t **NU_Server_select_receive(NU_Server_t *server, NU_Client_Socket_t **clients, size_t *size, unsigned int timeout);

/* Blocks for requested timeout or until one of the client sockets passed are available for sending. */
NU_Client_Socket_t **NU_Server_select_send(NU_Server_t *server, NU_Client_Socket_t **clients, size_t *size, unsigned int timeout);

/* Returns a string representation about this client, including but not limited to:
   1) Server's port number binded to as well as local IP.
   2) List of all client's currently connected to, as well as messages and data sent/received from each individually.
   3) Total amount of data and messages sent/received. */
char *NU_Server_about(NU_Server_t *server);

/* The server will no longer be accepting current connections, but will continue dealing with it's
   current connections until the time specified ellapses, upon which it will close all connections. */
int NU_Server_shutdown(NU_Server_t *server, const char *message);

/* Disconnect the server from the client. */
int NU_Server_disconnect(NU_Server_t *server, NU_Client_Socket_t *client, const char *message);

/* The server will immediately close all connections, free up all resources, and destroy itself. */
int NU_Server_destroy(NU_Server_t *server, const char *message);

#endif /* endif NET_UTILS_SERVER_H */