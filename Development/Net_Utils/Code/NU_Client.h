#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <Net_Utils.h>

typedef struct {
   /// Socket associated with this server.
   int sockfd;
   /// Keeps track of data used.
   NU_Collective_Data_t data;
   /// Keeps track of the hostname currently connected to.
   char host_name[100];
   /// Port number connecting through.
   char port_num[6];
   /// The timestamp determining when the client was created.
   char *timestamp;
   /// Determines whether or not the client is currently connected or not.
   unsigned char is_connected;
   /// Stores bytes read into this to eliminate constant arbitrary allocations.
   NU_Bounded_Buffer_t *bbuf;
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

#endif /* endif NET_UTILS_CLIENT_H */