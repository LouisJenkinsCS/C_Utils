/* Networking_Utils.h */
#ifndef Networking_Utils_Processed
#define Networking_Utils_Processed

#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

/* Find a use for this. Should be passed to whatever 
   functions have an option to block or not, I.E Accept. */
#define ASYNC 1 << 0 

/* The below will be the basic templates for a server and should, emphasis here,
   SHOULD be 100% optional! Do not force this, each function should use
   have full parameters, create a wrapper for the specific Server, Client and HTTP struct functions. */

typedef struct Server_Struct Server_T
typedef struct Client_Struct Client_T
typedef struct HTTP_Struct HTTP_T

struct Server_Struct {
	// Implement
}

struct Client_Struct {
	// Implement
}

struct HTTP_Struct {
	// Implement
}

/* This function should create the socket, then return
   it initialized. Requirements: A) Should check if it
   fails, and if so, return some resembling an error
   message. B) Should have parameter passing to set
   the parameters to be passed for socket. */
int Networking_Utils_Socket_Create(void);

/* This function should initialize the sockaddr_in and
   cast to a sockaddr after setting the appropriate
   members to appropriate information. Requirements: 
   A) Must be generic enough to NOT rely on a client_t
   structure passed to it. B) Must be generic enough
   that information can be passed and handled without
   overcomplicating things. C) Must check for NULL returns
   and handdle appropriately.  */
int Networking_Utils_Socket_Connect(const char *ip, unsigned int port_num, int sin_family);

/* This function should bind the socket to the port number passed
   and should appropriately fill out the sockaddr_in struct's members
   and return whether or not it succeeds or not. */
int Networking_Utils_Bind(int socket, int port_num, int sin_family, int s_addr);

/* Should set the socket to listen for a max of the passed queue_size
   and return appropriately on errors. */
int Networking_Utils_Socket_Listen(int socket, size_t queue_size);

/* Should return the first sockaddr_in accepted, A.K.A client. 
   Should have a parameter for whether to block or not. Also should
   handle returning on error (I.E NULL).*/
struct sockaddr_in *Networking_Utils_Socket_Accept(int socket, int parameter);

/* As the name implies, should close the socket, returning whether
   or not it was successful or not. */
int Networking_Utils_Socket_Close(int socket);

/* Implement the rest! */
#endif /* !Networking_Utils_Processed */