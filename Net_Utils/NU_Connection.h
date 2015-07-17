#ifndef NU_CONNECTION_H
#define NU_CONNECTION_H

#include <NU_Helper.h>

typedef struct NU_Connection_t {
   /// Socket file descriptor associated with host.
   volatile int sockfd;
   /// The IP Address of the host connected to.
   char ip_addr[INET_ADDRSTRLEN];
   /// Port number that the host is bound to.
   unsigned int port;
   /// Read-Write lock to use for synchronization if initialized.
   pthread_rwlock_t *lock;
   /// A reusable buffer for each connection.
   volatile unsigned char in_use;
} NU_Connection_t;

// Implement
NU_Connection_t *NU_Connection_create(unsigned char init_locks, MU_Logger_t *logger);

// Implement
size_t NU_Connection_send(NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_receive(NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement
size_t NU_Connection_receive_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger);

// Implement. Note to self: Needs to externally locked before calling.
NU_Connection_t *NU_Connection_reuse(NU_Connection_t **connections, size_t size, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);

// Implement
NU_Connection_t **NU_Connection_select_receive(NU_Connection_t **connections, size_t *size, unsigned int timeout, MU_Logger_t *logger);

// Implement
NU_Connection_t **NU_Connection_select_send(NU_Connection_t **connections, size_t *size, unsigned int timeout, MU_Logger_t *logger);


// Implement
int NU_Connection_get_sockfd(NU_Connection_t *conn, MU_Logger_t *logger);

// Implement
int NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd, MU_Logger_t *logger);

// Implement
const char *NU_Connection_get_ip_addr(NU_Connection_t *conn, MU_Logger_t *logger);

// Implement
int NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr, MU_Logger_t *logger);

// Implement
unsigned int NU_Connection_get_port(NU_Connection_t *conn, MU_Logger_t *logger);

// Implement
int NU_Connection_set_port(NU_Connection_t *conn, unsigned int port, MU_Logger_t *logger);

// Implement
char *NU_Connection_to_string(NU_Connection_t *connection, MU_Logger_t *logger);

// Implement
int NU_Connection_is_valid(NU_Connection_t *conn, MU_Logger_t *logger);

int NU_Connection_in_use(NU_Connection_t *conn, MU_Logger_t *logger);

int NU_Connection_init(NU_Connection_t *conn, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);

// Implement
int NU_Connection_disconnect(NU_Connection_t *conn, MU_Logger_t *logger);

// Implement
int NU_Connection_destroy(NU_Connection_t *conn, MU_Logger_t *logger);

#endif /* END NU_CONNECTION_H */
