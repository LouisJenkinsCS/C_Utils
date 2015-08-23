#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <NU_Helper.h>
#include <NU_Connection.h>

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
 * 
 * @param initial_size
 * @param synchronized
 * @return 
 */
NU_Client_t *NU_Client_create(size_t initial_size, bool synchronized);

/**
 * 
 * @param client
 * @param host
 * @param port
 * @param timeout
 * @return 
 */
NU_Connection_t *NU_Client_connect(NU_Client_t *client, const char *host, unsigned int port, long long int timeout);

/**
 * 
 * @param client
 * @param connection
 * @return 
 */
bool NU_Client_disconnect(NU_Client_t *client, NU_Connection_t *connection);

/**
 * 
 * @param client
 * @param message
 * @param ...
 * @return 
 */
bool NU_Client_log(NU_Client_t *client, const char *message, ...);

/**
 * 
 * @param client
 * @return 
 */
bool NU_Client_shutdown(NU_Client_t *client);

/**
 * 
 * @param client
 * @return 
 */
bool NU_Client_destroy(NU_Client_t *client);

#endif /* endif NET_UTILS_CLIENT_H */