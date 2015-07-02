#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <NU_Helper.h>

/**
* @brief 
*/
typedef struct NU_Server_Socket_t {
   /// Socket file descriptor associated with host.
   volatile int sockfd;
   /// The IP Address of the host connected to.
   char ip_addr[INET_ADDRSTRLEN];
   /// Port number that the host is bound to.
   unsigned int port;
   /// A reusable buffer for each connection.
   NU_Bounded_Buffer_t *bbuf;
   /// The next server socket in the list.
   struct NU_Server_Socket_t *next;
} NU_Server_Socket_t;

typedef struct {
   /// Socket associated with this server.
   NU_Server_Socket_t *servers;
   /// Amount of servers currently connected to.
   size_t amount_of_servers;
   /// Keeps track of data used.
   NU_Collective_Data_t data;
} NU_Client_t;

/*  Creates a basic client template, fully initialized and connected to the host. */
NU_Client_t *NU_Client_create();

/* Connects the client to some host! */
NU_Server_Socket_t *NU_Client_connect(NU_Client_t *client, const char *host, unsigned int port, unsigned int is_udp, unsigned int timeout);

/* Sends data to the host, up to the given timeout. */
size_t NU_Client_send(NU_Client_t *client, NU_Server_Socket_t *server, const char *message, size_t msg_size, unsigned int timeout);

size_t NU_Client_send_file(NU_Client_t *client, NU_Server_Socket_t *server, FILE *file, size_t buffer_size, unsigned int is_binary, unsigned int timeout);

size_t NU_Client_receive_to_file(NU_Client_t *client, NU_Server_Socket_t *server, FILE *file, size_t buffer_size, unsigned int is_binary, unsigned int timeout);

NU_Server_Socket_t **NU_Client_select_send(NU_Client_t *client, NU_Server_Socket_t **servers, size_t *size, unsigned int timeout);

NU_Server_Socket_t **NU_Client_select_receive(NU_Client_t *client, NU_Server_Socket_t **servers, size_t *size, unsigned int timeout);

/* Receives data from the host, up to a given timeout. */
const char *NU_Client_receive(NU_Client_t *client, NU_Server_Socket_t *server, size_t buffer_size, unsigned int timeout);

/* Returns a string representation of the information about this client, including but not limited to:
   1) Host connected to and port number.
   2) Client's hostname and local IP
   3) Amount of data and messages sent.
   4) Etc.
*/
char *NU_Client_about(NU_Client_t *client);

int NU_Client_disconnect(NU_Client_t *client, NU_Server_Socket_t *server, const char *message);

int NU_Client_log(NU_Client_t *client, const char *message, ...);

/* Will shutdown the client's socket after the time given has ellapsed.
   The client is not freed nor deallocated memory and can be reused. */
int NU_Client_shutdown(NU_Client_t *client, const char *message);

/* The client immediately closes it's socket, free up all resources, and destroy itself. */
int NU_Client_destroy(NU_Client_t *client, const char *message);

#endif /* endif NET_UTILS_CLIENT_H */