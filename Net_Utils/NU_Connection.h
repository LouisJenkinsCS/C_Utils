#ifndef NU_CONNECTION_H
#define NU_CONNECTION_H

#include <MU_Logger.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdbool.h>
#include <arpa/inet.h>


/**
 * Object/Struct wrapper for a socket file decsriptor, which keeps track of
 * not only the sockfd, but the ip address used to be bind it, the port it is
 * connected to, and records whether or not it currently is in use, as well as a
 * rwlock for synchronization. It even keeps track of the logger used during it's
 * initialization, which it will log any errors and information to, if one is declared.
 * 
 * Note: This object is reusable and is best used with NU_Client or NU_Server which
 * automates this for you!
 */
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

/**
 * 
 * @param conn
 * @param file
 * @param timeout
 * @param flags
 * @return 
 */
NU_Connection_t *NU_Connection_create(bool init_locks, MU_Logger_t *logger);

/**
 * 
 * @param connections
 * @param size
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
 */
size_t NU_Connection_send(NU_Connection_t *conn, const void *buffer, size_t buf_size, long long int timeout, int flags);

/**
 * 
 * @param conn
 * @return 
 */
size_t NU_Connection_receive(NU_Connection_t *conn, void *buffer, size_t buf_size, long long int timeout, int flags);


/**
 * 
 * @param conn
 * @param ip_addr
 * @return 
 */
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, long long int timeout, int flags);


/**
 * 
 * @param conn
 * @param port
 * @return 
 */
size_t NU_Connection_receive_file(NU_Connection_t *conn, FILE *file, long long int timeout, int flags);

/**
 * 
 * @param conn
 * @param logger
 * @return 
 */
NU_Connection_t *NU_Connection_reuse(NU_Connection_t **connections, size_t size, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);

/**
 * 
 * @param conn
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
 */
int NU_Connection_select(NU_Connection_t ***receivers, size_t *r_size, NU_Connection_t ***senders, size_t *s_size, long long int timeout, MU_Logger_t *logger);

/**
 * 
 * @param conn
 * @return 
 */
int NU_Connection_get_sockfd(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @param sockfd
 * @return 
 */
bool NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd);

/**
 * 
 * @param conn
 * @param ip_addr
 * @return 
 */
const char *NU_Connection_get_ip_addr(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @param port
 * @return 
 */
bool NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr);

/**
 * 
 * @param conn
 * @param logger
 * @return 
 */
unsigned int NU_Connection_get_port(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_set_port(NU_Connection_t *conn, unsigned int port);

/**
 * 
 * @param conn
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
 */
MU_Logger_t *NU_Connection_get_logger(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_set_logger(NU_Connection_t *conn, MU_Logger_t *logger);

/**
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_is_valid(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_in_use(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
 */
bool NU_Connection_init(NU_Connection_t *conn, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);

/**
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_disconnect(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_destroy(NU_Connection_t *conn);

#endif /* END NU_CONNECTION_H */
