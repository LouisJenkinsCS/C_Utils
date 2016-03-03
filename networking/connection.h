#ifndef NU_CONNECTION_H
#define NU_CONNECTION_H

#include <stdarg.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdatomic.h>

#include "../io/logger.h"

struct c_utils_connection;

#ifdef C_UTILS_USE_POSIX_STD
typedef struct c_utils_connection connection_t;
#define connection_init(...) c_utils_connection_init(__VA_ARGS__)
#define connection_create(...) c_utils_connection_create(__VA_ARGS__)
#define connection_send(...) c_utils_connection_send(__VA_ARGS__)
#define connection_send_file(...) c_utils_connection_send_file(__VA_ARGS__)
#define connection_receive(...) c_utils_connection_receive(__VA_ARGS__)
#define connection_receive_file(...) c_utils_connection_receive_file(__VA_ARGS__)
#define connection_select(...) c_utils_connection_select(__VA_ARGS__)
#define connection_disconnect(...) c_utils_connection_disconnect(__VA_ARGS__)
#define connection_reuse(...) c_utils_connection_reuse(__VA_ARGS__)
#define connection_set_sockfd(...) connection_set_sockfd(__VA_ARGS__)
#define connection_get_sockfd(...) c_utils_connection_get_sockfd(__VA_ARGS__)
#define connection_set_port(...) c_utils_connection_set_port(__VA_ARGS__)
#define connection_get_port(...) c_utils_connection_get_port(__VA_ARGS__)
#define connection_set_logger(...) c_utils_connection_set_logger(__VA_ARGS__)
#define connection_get_logger(...) c_utils_connection_get_logger(__VA_ARGS__)
#define connection_set_ip_addr(...) c_utils_connection_set_ip_addr(__VA_ARGS__)
#define connection_get_ip_addr(...) c_utils_connection_get_ip_addr(__VA_ARGS__)
#define connection_in_use(...) c_utils_connection_in_use(__VA_ARGS__)
#define connection_destroy(...) c_utils_connection_destroy(__VA_ARGS__)
#endif


/**
 * Represents a socket file descriptor and all it's affiliated information, I.E the ones used
 * to initialize this structure (sockfd, ip_addr, port), as well as a meants to allow safe
 * concurrent access. This structure is better off created with it's abstraction managers/factory,
 * NU_Server and NU_Client.
 *
 * It also keeps track of the logger used to initialize it, which it logs any errors to. It also supports
 * concurrent R/W access for any send/receive calls. Any errors it encounters will be logged.
 */
typedef struct c_utils_connection_t {
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
   struct c_utils_logger *logger;
} c_utils_connection_t;

/**
 * Create a new instance that is not connected to an endpoint. 
 * @param synchronized If true, R/W lock will be used.
 * @param logger The logger to log any information to if not left NULL.
 * @return A new instance of c_utils_connection, not connected.
 */
struct c_utils_connection *c_utils_connection_create(bool synchronized, struct c_utils_logger *logger);

/**
 * If the conn is connected, it will attempt to send the contents in buffer up to buf_size
 * to it's connected end-point, until either it succeeds or timeout has ellapsed. Any flags passed
 * will be directly used in the call to send().
 * @param conn Connection instance.
 * @param buffer Buffer contents to send.
 * @param buf_size The size of the buffer, or rather how much of the buffer it will attempt to send.
 * @param timeout Max amount of time to attempt to send. Infinite if -1
 * @param flags Flags to pass to send().
 * @return Amount of bytes sent, 0 if it fails to send any (either timeout or socket error);
 */
size_t c_utils_connection_send(struct c_utils_connection *conn, const void *buffer, size_t buf_size, long long int timeout, int flags);

/**
 * If the conn is connected, it will attempt to fill it's buffer up to buf_size with what is received from
 * it's endpoint, until either it receives data on the connection or the timeout ellapses. Any flags passed will
 * be directly used in the call to recv().
 * @param conn Connection instance.
 * @param buffer Buffer to receive into.
 * @param buf_size The size of the buffer, or rather how much of the buffer it will attempt to fill.
 * @param timeout Max amount of time to attempt to receive. Infinite if -1
 * @param flags Flags to pass to recv().
 * @return Amount of bytes read, 0 if it fails to read any (either timeout or socket error or not connected);
 */
size_t c_utils_connection_receive(struct c_utils_connection *conn, void *buffer, size_t buf_size, long long int timeout, int flags);


/**
 * If the conn is connected to an endpoint, it will attempt to send all of the contents of the file until
 * succeeds or timeout ellapses. Any flags passed will be directly passed in the call to send().
 * @param conn Connection instance.
 * @param file File to send.
 * @param timeout Maximum timeout to attempt for. Infinite if -1
 * @param flags Flags to pass to send().
 * @return Amount of bytes sent, 0 if it fails to send any.
 */
size_t c_utils_connection_send_file(struct c_utils_connection *conn, FILE *file, long long int timeout, int flags);


/**
 * If the conn is connected to an endpoint, it will attempt to read as much information to the file as possible,
 * until either it receives data on the connection or the timeout ellapses. Any flags passed will be directly
 * used in the call to recv().
 * @param conn Connection instance
 * @param file File to receive into.
 * @param timeout Maximum timeout to attempt for. Infinite if -1
 * @param flags The flags to pass to recv().
 * @return Amount of bytes read, 0 if it fails to read any.
 */
size_t c_utils_connection_receive_file(struct c_utils_connection *conn, FILE *file, long long int timeout, int flags);

/**
 * A helper function meant to take an array of c_utils_connection instance, and attempt to find one which is not currently
 * in use. If it does, it will take the already-connected sockfd and affiliated information and set it as it's current endpoint.
 * @param conn Connection instance.
 * @param size The size of the array.
 * @param sockfd The already-connected socket file descriptor.
 * @param port The port the sockfd is connected to.
 * @param ip_addr The IP address of the sockfd.
 * @param logger The new logger to log any and all errors to.
 * @return An unused connection fully initialized if possible, null on error or if there is no available to instance to reuse.
 */
struct c_utils_connection *c_utils_connection_reuse(struct c_utils_connection **connections, size_t size, int sockfd,
   unsigned int port, const char *ip_addr, struct c_utils_logger *logger);

/**
 * A wrapper function for the select statement which will work with this abstraction. It takes the pointer to an array of connections to receive,
 * a pointer to the receiver's original size, a pointer an array of connections to send, and a pointer to the sender's original size. It then blocks
 * until timeout or until at least one of the connections's endpoints can receive/send data. It returns how many connections are ready, and the arrays
 * are filled to represent which are ready, similar to how select() can take a file descriptor set and return what is ready, with the pointer their respective
 * sizes being filled out appropriately. Either receivers or senders can be NULL, but not both.
 * @param receivers Pointer to an array of receivers, can be NULL if senders is declared. This array is replaced with what is available, NULL if none.
 * @param r_size Pointer to the size of the receiver array, and is used to return the size of the array of ready receivers.
 * @param senders Pointer to an array of senders, can be NULL if receivers is declared. This array is replaced with what is available, NULL if none.
 * @param s_size Pointer to the size of the sender's array, and is used to return the size of the array of ready senders.
 * @param timeout Maximum timeout to block for, infinite if -1.
 * @param logger Logger to log any messages to if declared.
 * @return Amount of ready receivers/senders.
 */
int c_utils_connection_select(struct c_utils_connection ***receivers, size_t *r_size, struct c_utils_connection ***senders, size_t *s_size,
   long long int timeout, struct c_utils_logger *logger);

/**
 * Obtains the socket file descriptor.
 * @param conn Instance of connection.
 * @return Socket file descriptor of the connection.
 */
int c_utils_connection_get_sockfd(struct c_utils_connection *conn);

/**
 * Sets the socket file descriptor for an instance of Connection.
 * @param conn Instance of connection.
 * @param sockfd Socket file descriptor.
 * @return true if successfull, NULL if not.
 */
bool c_utils_connection_set_sockfd(struct c_utils_connection *conn, int sockfd);

/**
 * Obtains the IP Address.
 * @param conn Instance of connection.
 * @return IP Address of connection.
 */
const char *c_utils_connection_get_ip_addr(struct c_utils_connection *conn);

/**
 * Sets the IP Address for an instance of Connection.
 * @param conn Instance of connection.
 * @param ip_addr IP Address.
 * @return True on success, False on error.
 */
bool c_utils_connection_set_ip_addr(struct c_utils_connection *conn, const char *ip_addr);

/**
 * Obtains the port.
 * @param conn Instance of connection.
 * @return Port number.
 */
unsigned int c_utils_connection_get_port(struct c_utils_connection *conn);

/**
 * Set the port.
 * @param conn Instance of connection.
 * @return True on success, false on error.
 */
bool c_utils_connection_set_port(struct c_utils_connection *conn, unsigned int port);

/**
 * Obtains logger used.
 * @param conn Instance of connection.
 * @return Logger used by connection.
 */
struct c_utils_logger *c_utils_connection_get_logger(struct c_utils_connection *conn);

/**
 * Sets the logger used.
 * @param conn Instance of connection.
 * @return True on success, false on error.
 */
bool c_utils_connection_set_logger(struct c_utils_connection *conn, struct c_utils_logger *logger);

/**
 * Determines if the connection is currently in use.
 * @param conn Instance of connection.
 * @return Whether or not it is in use.
 */
bool c_utils_connection_in_use(struct c_utils_connection *conn);

/**
 * Initializes a connection object with it's parameters.
 * @param conn Instance of connection.
 * @param sockfd Socket File Descriptor.
 * @param port Port number.
 * @param ip_addr IP Address.
 * @param logger Logger.
 * @return True if successful, false on error.
 */
bool c_utils_connection_init(struct c_utils_connection *conn, int sockfd, unsigned int port, const char *ip_addr, struct c_utils_logger *logger);

/**
 * Disconnects the connection from it's endpoint.
 * @param conn Instance of connection.
 * @return True if successful, false on error.
 */
bool c_utils_connection_disconnect(struct c_utils_connection *conn);

/**
 * Destroys the connection, disconnecting it if it is currently connected.
 * @param conn Instance of connection.
 * @return True on success, false on error.
 */
bool c_utils_connection_destroy(struct c_utils_connection *conn);

#endif /* END NU_CONNECTION_H */
