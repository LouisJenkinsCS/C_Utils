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
   volatile bool in_use;
   /// Logger associated with each connection.
   MU_Logger_t *logger;
} NU_Connection_t;


NU_Connection_t *NU_Connection_create(bool init_locks, MU_Logger_t *logger);


size_t NU_Connection_send(NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout, int flags);


size_t NU_Connection_receive(NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout, int flags);


/// Sent in BUFSIZ buffers
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, unsigned int timeout, int flags);


///  Written in blksize chunks
size_t NU_Connection_receive_file(NU_Connection_t *conn, FILE *file, unsigned int timeout, int flags);


NU_Connection_t *NU_Connection_reuse(NU_Connection_t **connections, size_t size, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);


int NU_Connection_select(NU_Connection_t ***receivers, size_t *r_size, NU_Connection_t ***senders, size_t *s_size, unsigned int timeout, MU_Logger_t *logger);



int NU_Connection_get_sockfd(NU_Connection_t *conn);


bool NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd);


const char *NU_Connection_get_ip_addr(NU_Connection_t *conn);


bool NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr);


unsigned int NU_Connection_get_port(NU_Connection_t *conn);


bool NU_Connection_set_port(NU_Connection_t *conn, unsigned int port);


MU_Logger_t *NU_Connection_get_logger(NU_Connection_t *conn);


bool NU_Connection_set_logger(NU_Connection_t *conn, MU_Logger_t *logger);


bool NU_Connection_is_valid(NU_Connection_t *conn);


bool NU_Connection_in_use(NU_Connection_t *conn);


bool NU_Connection_init(NU_Connection_t *conn, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);


bool NU_Connection_disconnect(NU_Connection_t *conn);


bool NU_Connection_destroy(NU_Connection_t *conn);

#endif /* END NU_CONNECTION_H */
