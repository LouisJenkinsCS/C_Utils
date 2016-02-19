#ifndef NU_CONNECTION_H
#define NU_CONNECTION_H

#include <MU_Logger.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdbool.h>
#include <arpa/inet.h>


/**
<<<<<<< HEAD
 * Object/Struct wrapper for a socket file decsriptor, which keeps track of
 * not only the sockfd, but the ip address used to be bind it, the port it is
 * connected to, and records whether or not it currently is in use, as well as a
 * rwlock for synchronization. It even keeps track of the logger used during it's
 * initialization, which it will log any errors and information to, if one is declared.
 * 
 * Note: This object is reusable and is best used with NU_Client or NU_Server which
 * automates this for you!
=======
 * Represents a socket file descriptor and all it's affiliated information, I.E the ones used
 * to initialize this structure (sockfd, ip_addr, port), as well as a meants to allow safe
 * concurrent access. This structure is better off created with it's abstraction managers/factory,
 * NU_Server and NU_Client.
 *
 * It also keeps track of the logger used to initialize it, which it logs any errors to. It also supports
 * concurrent R/W access for any send/receive calls. Any errors it encounters will be logged.
>>>>>>> development
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
<<<<<<< HEAD
 * 
 * @param conn
 * @param file
 * @param timeout
 * @param flags
 * @return 
=======
 * Create a new instance that is not connected to an endpoint. 
 * @param init_locks If true, R/W lock will be used.
 * @param logger The logger to log any information to if not left NULL.
 * @return A new instance of NU_Connection_t, not connected.
>>>>>>> development
 */
NU_Connection_t *NU_Connection_create(bool init_locks, MU_Logger_t *logger);

/**
<<<<<<< HEAD
 * 
 * @param connections
 * @param size
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
=======
 * If the conn is connected, it will attempt to send the contents in buffer up to buf_size
 * to it's connected end-point, until either it succeeds or timeout has ellapsed. Any flags passed
 * will be directly used in the call to send().
 * @param conn Connection instance.
 * @param buffer Buffer contents to send.
 * @param buf_size The size of the buffer, or rather how much of the buffer it will attempt to send.
 * @param timeout Max amount of time to attempt to send. Infinite if -1
 * @param flags Flags to pass to send().
 * @return Amount of bytes sent, 0 if it fails to send any (either timeout or socket error);
>>>>>>> development
 */
size_t NU_Connection_send(NU_Connection_t *conn, const void *buffer, size_t buf_size, long long int timeout, int flags);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
=======
 * If the conn is connected, it will attempt to fill it's buffer up to buf_size with what is received from
 * it's endpoint, until either it receives data on the connection or the timeout ellapses. Any flags passed will
 * be directly used in the call to recv().
 * @param conn Connection instance.
 * @param buffer Buffer to receive into.
 * @param buf_size The size of the buffer, or rather how much of the buffer it will attempt to fill.
 * @param timeout Max amount of time to attempt to receive. Infinite if -1
 * @param flags Flags to pass to recv().
 * @return Amount of bytes read, 0 if it fails to read any (either timeout or socket error or not connected);
>>>>>>> development
 */
size_t NU_Connection_receive(NU_Connection_t *conn, void *buffer, size_t buf_size, long long int timeout, int flags);


/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param ip_addr
 * @return 
=======
 * If the conn is connected to an endpoint, it will attempt to send all of the contents of the file until
 * succeeds or timeout ellapses. Any flags passed will be directly passed in the call to send().
 * @param conn Connection instance.
 * @param file File to send.
 * @param timeout Maximum timeout to attempt for. Infinite if -1
 * @param flags Flags to pass to send().
 * @return Amount of bytes sent, 0 if it fails to send any.
>>>>>>> development
 */
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, long long int timeout, int flags);


/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param port
 * @return 
=======
 * If the conn is connected to an endpoint, it will attempt to read as much information to the file as possible,
 * until either it receives data on the connection or the timeout ellapses. Any flags passed will be directly
 * used in the call to recv().
 * @param conn Connection instance
 * @param file File to receive into.
 * @param timeout Maximum timeout to attempt for. Infinite if -1
 * @param flags The flags to pass to recv().
 * @return Amount of bytes read, 0 if it fails to read any.
>>>>>>> development
 */
size_t NU_Connection_receive_file(NU_Connection_t *conn, FILE *file, long long int timeout, int flags);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param logger
 * @return 
=======
 * A helper function meant to take an array of NU_Connection_t instance, and attempt to find one which is not currently
 * in use. If it does, it will take the already-connected sockfd and affiliated information and set it as it's current endpoint.
 * @param conn Connection instance.
 * @param size The size of the array.
 * @param sockfd The already-connected socket file descriptor.
 * @param port The port the sockfd is connected to.
 * @param ip_addr The IP address of the sockfd.
 * @param logger The new logger to log any and all errors to.
 * @return An unused connection fully initialized if possible, null on error or if there is no available to instance to reuse.
>>>>>>> development
 */
NU_Connection_t *NU_Connection_reuse(NU_Connection_t **connections, size_t size, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
=======
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
>>>>>>> development
 */
int NU_Connection_select(NU_Connection_t ***receivers, size_t *r_size, NU_Connection_t ***senders, size_t *s_size, long long int timeout, MU_Logger_t *logger);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
=======
 * Obtains the socket file descriptor.
 * @param conn Instance of connection.
 * @return Socket file descriptor of the connection.
>>>>>>> development
 */
int NU_Connection_get_sockfd(NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param sockfd
 * @return 
=======
 * Sets the socket file descriptor for an instance of Connection.
 * @param conn Instance of connection.
 * @param sockfd Socket file descriptor.
 * @return true if successfull, NULL if not.
>>>>>>> development
 */
bool NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param ip_addr
 * @return 
=======
 * Obtains the IP Address.
 * @param conn Instance of connection.
 * @return IP Address of connection.
>>>>>>> development
 */
const char *NU_Connection_get_ip_addr(NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param port
 * @return 
=======
 * Sets the IP Address for an instance of Connection.
 * @param conn Instance of connection.
 * @param ip_addr IP Address.
 * @return True on success, False on error.
>>>>>>> development
 */
bool NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param logger
 * @return 
=======
 * Obtains the port.
 * @param conn Instance of connection.
 * @return Port number.
>>>>>>> development
 */
unsigned int NU_Connection_get_port(NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
=======
 * Set the port.
 * @param conn Instance of connection.
 * @return True on success, false on error.
>>>>>>> development
 */
bool NU_Connection_set_port(NU_Connection_t *conn, unsigned int port);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
=======
 * Obtains logger used.
 * @param conn Instance of connection.
 * @return Logger used by connection.
>>>>>>> development
 */
MU_Logger_t *NU_Connection_get_logger(NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
=======
 * Sets the logger used.
 * @param conn Instance of connection.
 * @return True on success, false on error.
>>>>>>> development
 */
bool NU_Connection_set_logger(NU_Connection_t *conn, MU_Logger_t *logger);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
 */
bool NU_Connection_is_valid(NU_Connection_t *conn);

/**
 * 
 * @param conn
 * @return 
=======
 * Determines if the connection is currently in use.
 * @param conn Instance of connection.
 * @return Whether or not it is in use.
>>>>>>> development
 */
bool NU_Connection_in_use(NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @param sockfd
 * @param port
 * @param ip_addr
 * @param logger
 * @return 
=======
 * Initializes a connection object with it's parameters.
 * @param conn Instance of connection.
 * @param sockfd Socket File Descriptor.
 * @param port Port number.
 * @param ip_addr IP Address.
 * @param logger Logger.
 * @return True if successful, false on error.
>>>>>>> development
 */
bool NU_Connection_init(NU_Connection_t *conn, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
=======
 * Disconnects the connection from it's endpoint.
 * @param conn Instance of connection.
 * @return True if successful, false on error.
>>>>>>> development
 */
bool NU_Connection_disconnect(NU_Connection_t *conn);

/**
<<<<<<< HEAD
 * 
 * @param conn
 * @return 
=======
 * Destroys the connection, disconnecting it if it is currently connected.
 * @param conn Instance of connection.
 * @return True on success, false on error.
>>>>>>> development
 */
bool NU_Connection_destroy(NU_Connection_t *conn);

#endif /* END NU_CONNECTION_H */
