#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <NU_Helper.h>
#include <NU_Connection.h>

typedef struct {
   /// Socket associated with this server.
   NU_Connection_t *connections;
   /// Amount of servers currently connected to.
   size_t amount_of_connections;
   /// Keeps track of data used.
   NU_Collective_Data_t data;
} NU_Client_t;

/*  Creates a basic client template, fully initialized and connected to the host. */
NU_Client_t *NU_Client_create();

/* Connects the client to some host! */
NU_Connection_t *NU_Client_connect(NU_Client_t *client, const char *host, unsigned int port, unsigned int timeout);

/* Sends data to the host, up to the given timeout. */
size_t NU_Client_send(NU_Client_t *client, NU_Connection_t *connection, const char *message, size_t msg_size, unsigned int timeout);

size_t NU_Client_send_file(NU_Client_t *client, NU_Connection_t *connection, FILE *file, size_t buffer_size, unsigned int timeout);

size_t NU_Client_receive_to_file(NU_Client_t *client, NU_Connection_t *connection, FILE *file, size_t buffer_size, unsigned int timeout);

NU_Connection_t **NU_Client_select_send(NU_Client_t *client, NU_Connection_t **connectionss, size_t *size, unsigned int timeout);

NU_Connection_t **NU_Client_select_receive(NU_Client_t *client, NU_Connection_t **connectionss, size_t *size, unsigned int timeout);

/* Receives data from the host, up to a given timeout. */
const char *NU_Client_receive(NU_Client_t *client, NU_Connection_t *connection, size_t buffer_size, unsigned int timeout);

/* Returns a string representation of the information about this client, including but not limited to:
   1) Host connected to and port number.
   2) Client's hostname and local IP
   3) Amount of data and messages sent.
   4) Etc.
*/
char *NU_Client_about(NU_Client_t *client);

int NU_Client_disconnect(NU_Client_t *client, NU_Connection_t *connection, const char *message);

int NU_Client_log(NU_Client_t *client, const char *message, ...);

/* Will shutdown the client's socket after the time given has ellapsed.
   The client is not freed nor deallocated memory and can be reused. */
int NU_Client_shutdown(NU_Client_t *client, const char *message);

/* The client immediately closes it's socket, free up all resources, and destroy itself. */
int NU_Client_destroy(NU_Client_t *client, const char *message);

#endif /* endif NET_UTILS_CLIENT_H */