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
   /// Read-Write lock to use for synchronization if initialized.
   pthread_rwlock_t lock;
   /// A reusable buffer for each connection.
   NU_Buffer_t *buf;
   /// The next server socket in the list.
   NU_Connect_t *next;
} NU_Connection_t;

// Implement
NU_Connection_t *NU_Connection_create(NU_Connection_Type_t type, unsigned char init_locks, MU_Logger_t *logger);

// Implement
size_t NU_Connection_send(NU_Connect_t *conn, const void *message, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
const void *NU_Connect_receive(NU_Connect_t *conn, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_receive_file(NU_Connect_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
char *NU_Connection_to_string(NU_Connection_t *connection);

// Implement
int NU_Connection_get_sockfd(NU_Connection_t *conn);

// Implement
void NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd);

// Implement
const char *NU_Connection_get_ip_addr(NU_Connection_t *conn);

// Implement
void NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr);

// Implement
unsigned int NU_Connection_get_port(NU_Connection_t *conn);

// Implement
void NU_Connection_set_port(NU_Connection_t *conn, unsigned int port);

// Implement
int NU_Connection_is_valid(NU_Connection_t *conn);

// Implement
void NU_Connection_disconnect(NU_Connection_t *conn);

// Implement
void NU_Connection_destroy(NU_Connection_t *conn);

#endif /* END NU_CONNECTION_H */