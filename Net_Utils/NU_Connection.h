#ifndef NU_CONNECTION_H
#define NU_CONNECTION_H
#include <NU_Helper.h>

/*
*  Connection is sort of a base-class for everything. If C was an OOP language, Client and Server would inherit from this, and HTTP
*  would inherit from Client and Server. It handles majority of the connections, although these functions should NOT be used by the user,
*  as they are externally synchronized to allow thread-safety.
*/

typedef enum {
   /// Client type of connection.
   NU_Client,
   /// Server type of connection.
   NU_Server,
   /// HTTP type of connection.
   NU_HTTP,
   /// A user defined connection type.
   NU_Custom
} NU_Connection_Type_t;

typedef struct NU_Connection_t {
   /// Socket file descriptor associated with host.
   volatile int sockfd;
   /// The IP Address of the host connected to.
   char ip_addr[INET_ADDRSTRLEN];
   /// Port number that the host is bound to.
   unsigned int port;
   /// The type of connection this is.
   NU_Connection_Type_t type;
   /// A reusable buffer for each connection.
   NU_Buffer_t *buf;
   /// The next server socket in the list.
   NU_Connect_t *next;
} NU_Connection_t;

// Implement
NU_Connection_t *NU_Connection_create(NU_Connection_Type_t type, unsigned char init_locks, MU_Logger_t *logger);

// Implement
size_t NU_Connection_send(NU_Connect_t *conn, const void *message, size_t msg_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
void *NU_Connect_receive(NU_Connect_t *conn, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_receive_file(NU_Connect_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
NU_Connection_t **NU_Connection_select_receive(NU_Connect_t **conn, size_t *size, unsigned int timeout, MU_Logger_t *logger);

// Implement
NU_Connection_t **NU_Connection_select_send(NU_Connect_t **conn, size_t *size, unsigned int timeout, MU_Logger_t *logger)

// Implement
char *NU_Connection_to_string(NU_Connection_t *connection);

// Implement. Note to self: Needs to externally locked before calling.
NU_Connection_t *NU_Connection_reuse(NU_Connection_t *head, MU_Logger_t *logger);

void NU_Connection_disconnect(NU_Connection_t *conn);

void NU_Connection_destroy(NU_Connection_t *conn);

#endif /* END NU_CONNECTION_H */