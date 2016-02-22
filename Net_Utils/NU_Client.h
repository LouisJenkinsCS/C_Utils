#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <NU_Helper.h>
#include <NU_Connection.h>

#ifdef C_UTILS_USE_POSIX_STD
#define client_t NU_Client_t
#define client_create(...) NU_Client_create(__VA_ARGS__)
#define client_connect(...) NU_Client_connect(__VA_ARGS__)
#define client_disconnect(...) NU_Client_disconnect(__VA_ARGS__)
#define client_log(...) NU_Client_log(__VA_ARGS__)
#define client_shutdown(...) NU_Client_shutdown(__VA_ARGS__)
#define client_destroy(...) NU_Client_destroy(__VA_ARGS__)
#endif

/*
	NU_Connection_t is a simple manager for NU_Connection_t instances, creating and managing a pool for you
	to recycle for efficiency, with optional synchronized access for thread-safety. It allows you to do the
	following:

	* Connect to an End Point
	* Efficiently Recycle connections to End Points.
	* Logs any errors, warnings and information to a log file.
	* Cleanly destroy and shutdown existing connections.

	Notes: You should never, ever, EVER free the NU_Connection_t instances returned from any NU_Client_t instance.
	It will invoke undefined behaviour and most likely a segmentation fault!
*/

/**
 * The manager for connections, recycles existing connections to be re-used.
 */
typedef struct {
   /// Socket associated with this server.
   NU_Connection_t **connections;
   /// Amount of servers currently connected to.
   size_t amount_of_connections;
   /// Lock used for synchronization and thread safety.
   pthread_mutex_t *lock;
   /// Whether or not to initialize locks on everything.
   bool synchronized;
} NU_Client_t;

/**
 * Create and initialize a NU_Client_t instance, creating a pool of initial_size connection objects, minimum
 * of 1. If synchronized, it will synchronize access to the NU_Client_t instance, meaning whenever you
 * recycle a connection, retire a connection, disconnect, or connect, etc. 
 * @param initial_size Initial pool size of NU_Connection_t instances.
 * @param synchronized If true, all access will be synchronized and thread-safe.
 * @return A fully initialized instance of NU_Client_t, or NULL on memory error.
 */
NU_Client_t *NU_Client_create(size_t initial_size, bool synchronized);

/**
 * Creates (or recylces) a connection to the host and port end-point, and will block until either
 * successful or the given timeout ellapses.
 * @param client Instance of NU_Client_t.
 * @param host Host end-point to connect to.
 * @param port Port of the host to connect to.
 * @param timeout Maximum amount of time to attempt to connect. If -1, indefinitely.
 * @return A connected NU_Connection_t end-point, or NULL if error or timeout ellapses.
 */
NU_Connection_t *NU_Client_connect(NU_Client_t *client, const char *host, unsigned int port, long long int timeout);

/**
 * Disconnects the connection and marks as free to recycle in the NU_Connection_t pool.
 * @param client Instance of NU_Client_t.
 * @param connection Instance of a NU_Connection_t returned from this instance of NU_Client_t to disconnect.
 * @return true on success, false on error.
 */
bool NU_Client_disconnect(NU_Client_t *client, NU_Connection_t *connection);

/**
 * Logs the passed message to the client's log-file.
 * @param client Instance of NU_Client_t.
 * @param message Message to log.
 * @param ... Variadic Arguments to pass to the message format.
 * @return true on success, false on error.
 */
bool NU_Client_log(NU_Client_t *client, const char *message, ...);

/**
 * Shutdown all connections in the NU_Client_t instance.
 * @param client Instance of NU_Client_t.
 * @return true on success, false on error.
 */
bool NU_Client_shutdown(NU_Client_t *client);

/**
 * Shutdown and destroy all connections in the NU_Client_t instance.
 * @param client Instance of NU_Client_t.
 * @return true on success, false on error.
 */
bool NU_Client_destroy(NU_Client_t *client);

#endif /* endif NET_UTILS_CLIENT_H */