#ifndef NET_UTILS_CLIENT_H
#define NET_UTILS_CLIENT_H

#include <NU_Helper.h>
#include <NU_Connection.h>

typedef struct {
   /// Socket associated with this server.
   NU_Connection_t **connections;
   /// Amount of servers currently connected to.
   size_t amount_of_connections;
   /// RWLock to ensure thread safety on modifying connections and amount;
   pthread_rwlock_t *lock;
   /// Whether or not to initialize locks on everything.
   bool is_threaded;
} NU_Client_t;

/*  Creates a basic client template, fully initialized and connected to the host. */
NU_Client_t *NU_Client_create(size_t initial_size, bool init_locks);

/* Connects the client to some host! */
NU_Connection_t *NU_Client_connect(NU_Client_t *client, const char *host, unsigned int port, unsigned int timeout);

bool NU_Client_disconnect(NU_Client_t *client, NU_Connection_t *connection);

bool NU_Client_log(NU_Client_t *client, const char *message, ...);

/* Will shutdown the client's socket after the time given has ellapsed.
   The client is not freed nor deallocated memory and can be reused. */
bool NU_Client_shutdown(NU_Client_t *client);

/* The client immediately closes it's socket, free up all resources, and destroy itself. */
bool NU_Client_destroy(NU_Client_t *client);

#endif /* endif NET_UTILS_CLIENT_H */