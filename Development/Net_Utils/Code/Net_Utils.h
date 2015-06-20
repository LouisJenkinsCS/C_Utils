#ifndef NET_UTILS_H
#define NET_UTILS_H

/* Client-Server general data structures below */

typedef struct {
   size_t messages_sent;
   /// Amount of messages received.
   size_t messages_received;
   /// Total amount of data sent.
   size_t bytes_sent;
   /// Total amount of data received.
   size_t bytes_received;
} NU_Collective_Data_t;

typedef struct {
   /// Container for buffer.
   char *buffer;
   /// Current size of buffer.
   size_t size;
   /// Start position for the buffer.
   unsigned int index;
} NU_Bounded_Buffer_t;

/* Client template and functions declared below! */

typedef struct {
   /// Socket associated with this server.
   int sockfd;
   /// Keeps track of data used.
   NU_Collective_Data_t data;
   /// Keeps track of the hostname currently connected to.
   char host_name[100];
   /// Port number connecting through.
   char port_num[5];
   /// The timestamp determining when the client was created.
   char *timestamp;
   /// Determines whether or not the client is currently connected or not.
   unsigned char is_connected;
   /// Stores bytes read into this to eliminate constant arbitrary allocations.
   NU_Bounded_Buffer_t *bounded_buffer;
;} NU_Client_t;

/*  Creates a basic client template, fully initialized and connected to the host. */
NU_Client_t *NU_Client_create(int flags);

/* Connects the client to some host! */
int MU_Client_connect(NU_Client_t *client, const char *host, const char *port, int flags);

/* Sends data to the host, up to the given timeout. */
int NU_Client_send(NU_Client_t *client, const char *message, unsigned int timeout);

/* Receives data from the host, up to a given timeout. */
const char *NU_Client_recieve(NU_Client_t *client, size_t buffer_size, unsigned int timeout);

/* Returns a string representation of the information about this client, including but not limited to:
   1) Host connected to and port number.
   2) Client's hostname and local IP
   3) Amount of data and messages sent.
   4) Etc.
*/
char *NU_Client_about(NU_Client_t *client);

/* Will shutdown the client's socket after the time given has ellapsed.
   The client is not freed nor deallocated memory and can be reused. */
int NU_Client_shutdown(NU_Client_t *client, unsigned int when);

/* The client immediately closes it's socket, free up all resources, and destroy itself. */
int NU_Client_destroy(NU_Client_t *client);

/* Server template and functions declared below! */

typedef struct {
   /// Socket associated with this server.
   int server;
   /// Array of clients currently connected to.
   NU_Client_t **clients;
   /// Size of array connected to.
   size_t amount_of_clients;
} NU_Server_t;

/* Create a fully initialized server that is unconnected. The socket used is
   bound to the passed port, but no connections are being accepted on creation. */
NU_Server_t *NU_Server_create(unsigned int port, int flags);

/* Accept new connections until the timeout ellapses, up to the given amount. The returned
   client should not be freed, and it is also managed by the server. */
NU_Client_t *NU_Server_accept(NU_Server_t *server, size_t amount,  unsigned int timeout);

/* Send data to the requested client. */
int NU_Server_send(NU_Server_t *server, NU_Client_t *client, char *message, unsigned int timeout);

/* Receives data from any of current connections. */
char *NU_Server_recieve(NU_Server_t *server, size_t buffer_size, unsigned int timeout);

/* Returns a string representation about this client, including but not limited to:
   1) Server's port number binded to as well as local IP.
   2) List of all client's currently connected to, as well as messages and data sent/received from each individually.
   3) Total amount of data and messages sent/received. */
char *NU_Server_about(NU_Server_t *server);

/* The server will no longer be accepting current connections, but will continue dealing with it's
   current connections until the time specified ellapses, upon which it will close all connections. */
int NU_Server_shutdown(NU_Server_t *server, unsigned int when);

/* The server will immediately close all connections, free up all resources, and destroy itself. */
int NU_Server_destroy(NU_Server_t *server);

/* HTTP template and functions declared below! */

typedef struct {
   /// TODO: Implement!
} NU_HTTP_t;

/* FTP template and functions declared below! */

typedef struct {
   /// TODO: Implement!
} NU_FTP_t;


#endif /* END NET_UTILS_H */