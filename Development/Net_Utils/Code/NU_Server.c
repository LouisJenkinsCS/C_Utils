#include <NU_Server.h>
#define MU_LOG_BSOCK_ERR(function, bsock) do {\
	MU_LOG_VERBOSE(logger, "bsock: port->\"%s\", sockfd->%d, has_next: %s\n", bsock->port, bsock->sockfd, bsock->next ? "True" : "False"); \
	MU_LOG_WARNING(logger, function# ": \"%s\"\n", strerror(-1)); \
} while(0)

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for NU_Server's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "NU_Server_Log.txt", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

/* Server-specific helper functions */

static NU_Client_Socket_t *reuse_existing_client(NU_Client_Socket_t *head){
	NU_Client_Socket_t *tmp_client = NULL;
	for(tmp_client = head; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(!tmp_client->sockfd) break;
	}
	MU_LOG_VERBOSE(logger, "Currently existing client?: %s\n", tmp_client ? "True" : "False");
	return tmp_client;
}

static int setup_bound_socket(NU_Bound_Socket_t *bsock, char *port){
	int i = 0, flag = 1;
	struct sockaddr_in my_addr;
	while(bsock->port[i] = port[i++]);
	bsock->port[5] = '\0';
	if((bsock->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		MU_LOG_BSOCK_ERR(socket, bsock);
		return 0;
	}
	if(setsockopt(bsock->sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
		MU_LOG_BSOCK_ERR(setsockopt, bsock);
		shutdown(bsock->sockfd, SHUT_RDWR);
		return 0;
	}	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(bsock->port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(bsock->sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		MU_LOG_BSOCK_ERR(bind, bsock);
		shutdown(bsock->sockfd, SHUT_RDWR);
		return 0;
	}
	if(listen(bsock->sockfd, queue_size) == -1){
		MU_LOG_BSOCK_ERR(listen, bsock);
		shutdown(bsock->sockfd, SHUT_RDWR);
		return 0;
	}
	return 1;
}

static NU_Bound_Socket_t *reuse_existing_socket(NU_Bound_Socket_t *head, char *port){
	NU_Bound_Socket_t *tmp_bsock = NULL;
	for(tmp_bsock = head; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(!tmp_bsock->sockfd) break;
	}
	MU_LOG_VERBOSE(logger, "Currently existing bsock?: %s\n", tmp_bsock ? "True" : "False");
	return tmp_bsock;
}

static void destroy_bound_socket(NU_Bound_Socket_t *head_bsock, NU_Bound_Socket_t *current_bsock){
	shutdown(current_bsock->sockfd, SHUT_RDWR);
	// If a bound socket is being destroyed, then the list of bound sockets must be updated.
	NU_Bound_Socket_t *tmp_bsock = NULL;
	if(head_bsock == current_bsock){ 
		head_bsock = NULL;
		free(current_bsock);
		return;
	}
	for(tmp_bsock = head_bsock; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(tmp_bsock->next == current_bsock) {
			tmp_bsock->next = current_bsock->next;
			break;
		}
	}
	free(current_bsock)
}

NU_Server_t *NU_Server_create(int flags){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	return server;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, char *port, size_t queue_size int flags){
	if(!server || !port || !queue_size) return NULL;
	int is_reused = 1;
	NU_Bound_Socket_t *bsock = reuse_existing_socket(server->sockets);
	if(!bsock){
		bsock = calloc(1, sizeof(NU_Bound_Socket_t));
		MU_ASSERT_RETURN(bsock, logger, NULL, "Was unable to allocate memory for bsock!\n");
		is_reused--;
	}
	if(!is_reused){
		if(!server->sockets) server->sockets = bsock;
		else {
			NU_Bound_Socket_t *tmp_bsock = NULL;
			for(tmp_bsock = server->sockets; tmp_bsock && tmp_bsock->next; tmp_bsock = tmp_bsock->next);
			tmp_bsock->next = bsock;
		}
	}
	if(!setup_bound_socket(bsock, port)){
		MU_LOG_WARNING(logger, "setup_bound_socket: \"was unable to setup bsock\"\n");
		return NULL;
	}
	server->amount_of_sockets++;
	return bsock;
}

int NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	if(!server || !bsock || !bsock->sockfd) return 0;
	if(shutdown(bsock->sockfd, SHUT_RD) == -1){
		MU_LOG_BSOCK_ERR(shutdown, bsock);
		return 0;
	}
	NU_Client_Socket_t *client = NULL;
	for(client = server->clients; client; client = client->next){
		if(client->port == bsock->port){
			NU_Server_disconnect(server, client);
		}
	}
	server->amount_of_sockets--;
	if(shutdown(bsock->sockfd, SHUT_RDWR) == -1){
		MU_LOG_BSOCK_ERR(shutdown, bsock);
		return 0;
	}
	return 1;
}

NU_Client_Socket_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	/// TODO: Create a function which will search for a non-connected (free) client and return that instead.
	int is_reused = 1;
	NU_Client_Socket_t *client = reuse_existing_client(server->clients);
	if(!client) {
		client = calloc(1, sizeof(NU_Client_Socket_t));
		MU_ASSERT_RETURN(client, logger, NULL, "Was unable to allocate memory for client!\n");
		is_reused--;
	}
	if(!is_reused){
		client->bbuf = calloc(1, sizeof(NU_Bounded_Buffer_t));
		// Add it to the list of clients. TODO: Make a function called add_client that handles this.
		if(!server->clients) server->clients = client;
		else {
			NU_Client_Socket_t *tmp_client = NULL;
			for(tmp_client = server->clients; tmp_client && tmp_client->next; tmp_client = tmp_client->next);
			tmp_client->next = client;
		}
	}
	struct sockaddr_in client_addr;
	size_t client_size = sizeof(struct sockaddr_in);
	if((client->sockfd = accept(bsock->sockfd, (struct sockaddr *)&client_addr, &client_size)) == -1){
		MU_LOG_BSOCK_ERR(accept, bsock);
		client->sockfd = 0;
		// Note that the client isn't freed and even if there is an error, it is still added to the list to be reused.
		return NULL;
	}
	if(!inet_ntop(AF_INET, &client_addr, client->ip_address, INET_ADDRSTRLEN)) MU_LOG_WARNING(logger, "inet_ntop: \"%s\"\n", strerror(-1));
	client->port = bsock->port;
	server->amount_of_clients++;
	return client;
}