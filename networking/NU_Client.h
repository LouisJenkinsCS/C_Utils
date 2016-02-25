#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <NU_Helper.h>
#include <NU_Connection.h>

/*
	c_utils_client is a simple manager for c_utils_connection instances, creating and managing a pool for you
	to recycle for efficiency, with optional synchronized access for thread-safety. It allows you to do the
	following:

	* Connect to an End Point
	* Efficiently Recycle connections to End Points.
	* Logs any errors, warnings and information to a log file.
	* Cleanly destroy and shutdown existing connections.

	Notes: You should never, ever, EVER free the c_utils_connection instances returned from any c_utils_client instance.
	It will invoke undefined behaviour and most likely a segmentation fault!
*/
struct c_utils_client;

#ifdef C_UTILS_USE_POSIX_STD
typedef struct c_utils_client client_t;
#define client_create(...) c_utils_client_create(__VA_ARGS__)
#define client_connect(...) c_utils_client_connect(__VA_ARGS__)
#define client_disconnect(...) c_utils_client_disconnect(__VA_ARGS__)
#define client_log(...) c_utils_client_log(__VA_ARGS__)
#define client_shutdown(...) c_utils_client_shutdown(__VA_ARGS__)
#define client_destroy(...) c_utils_client_destroy(__VA_ARGS__)
#endif


/**
 * Create and initialize a c_utils_client instance, creating a pool of initial_size connection objects, minimum
 * of 1. If synchronized, it will synchronize access to the c_utils_client instance, meaning whenever you
 * recycle a connection, retire a connection, disconnect, or connect, etc. 
 * @param initial_size Initial pool size of c_utils_connection instances.
 * @param synchronized If true, all access will be synchronized and thread-safe.
 * @return A fully initialized instance of c_utils_client, or NULL on memory error.
 */
struct c_utils_client *NU_Client_create(size_t initial_size, bool synchronized);

/**
 * Creates (or recylces) a connection to the host and port end-point, and will block until either
 * successful or the given timeout ellapses.
 * @param client Instance of c_utils_client.
 * @param host Host end-point to connect to.
 * @param port Port of the host to connect to.
 * @param timeout Maximum amount of time to attempt to connect. If -1, indefinitely.
 * @return A connected c_utils_connection end-point, or NULL if error or timeout ellapses.
 */
struct c_utils_connection *NU_Client_connect(struct c_utils_client *client, const char *host, unsigned int port, long long int timeout);

/**
 * Disconnects the connection and marks as free to recycle in the c_utils_connection pool.
 * @param client Instance of c_utils_client.
 * @param connection Instance of a c_utils_connection returned from this instance of c_utils_client to disconnect.
 * @return true on success, false on error.
 */
bool NU_Client_disconnect(struct c_utils_client *client, struct c_utils_connection *connection);

/**
 * Logs the passed message to the client's log-file.
 * @param client Instance of c_utils_client.
 * @param message Message to log.
 * @param ... Variadic Arguments to pass to the message format.
 * @return true on success, false on error.
 */
bool NU_Client_log(struct c_utils_client *client, const char *message, ...);

/**
 * Shutdown all connections in the c_utils_client instance.
 * @param client Instance of c_utils_client.
 * @return true on success, false on error.
 */
bool NU_Client_shutdown(struct c_utils_client *client);

/**
 * Shutdown and destroy all connections in the c_utils_client instance.
 * @param client Instance of c_utils_client.
 * @return true on success, false on error.
 */
bool NU_Client_destroy(struct c_utils_client *client);

#endif /* endif NET_UTILS_CLIENT_H */