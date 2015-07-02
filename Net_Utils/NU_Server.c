#include <NU_Server.h>

static MU_Logger_t *logger = NULL;

#define MU_LOG_BSOCK_ERR(function, bsock) do { \
	MU_LOG_VERBOSE(logger, "bsock: port->\"%d\", sockfd->%d, has_next: %s\n", bsock->port, bsock->sockfd, bsock->next ? "True" : "False"); \
	MU_LOG_WARNING(logger, #function ": \"%s\"\n", strerror(errno)); \
	bsock->sockfd = 0; \
} while(0)

#define MU_LOG_SERVER(message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

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

static void delete_all_clients(NU_Server_t *server){
	if(!server->clients) return;
	NU_Client_Socket_t *client = NULL;
	for(client = server->clients; client; client = server->clients){
		server->clients = client->next;
		server->amount_of_clients--;
		if(client->bbuf) free(client->bbuf->buffer);
		free(client->bbuf);
		free(client);
	}
}

static void delete_all_sockets(NU_Server_t *server){
	if(!server->sockets) return;
	NU_Bound_Socket_t *bsock = NULL;
	for(bsock = server->sockets; bsock; bsock = server->sockets){
		server->sockets = bsock->next;
		server->amount_of_sockets--;
		free(bsock);
	}
}

static char *bsock_to_string(NU_Bound_Socket_t *head){
	if(!head) return NULL;
	char *bsock_str = NULL;
	NU_Bound_Socket_t *bsock = NULL;
	for(bsock = head; bsock; bsock = bsock->next){
		char *old_str = bsock_str;
		asprintf(&bsock_str, "%s (port: %d, sockfd: %d) %s", old_str ? old_str : "", bsock->port, bsock->sockfd, bsock->next ? "," : "");
		free(old_str);
	}
	return bsock_str;
}

static char *clients_to_string(NU_Client_Socket_t *head){
	if(!head) return NULL;
	char *clients_str = NULL;
	NU_Client_Socket_t *client = NULL;
	for(client = head; client; client = client->next){
		char *old_str = clients_str;
		asprintf(&clients_str, "%s (sockfd: %d, ip_addr: %s, port: %u, bbuf: [init?: %s, size: %zu])",
			clients_str ? clients_str : "", client->sockfd, client->ip_addr, client->port, client->bbuf ? "True" : "False",
			(client->bbuf && client->bbuf->size ? client->bbuf->size : 0));
		free(old_str);
	}
	return clients_str;
}

static NU_Client_Socket_t *reuse_existing_client(NU_Client_Socket_t *head){
	NU_Client_Socket_t *tmp_client = NULL;
	for(tmp_client = head; tmp_client; tmp_client = tmp_client->next){
		if(!tmp_client->sockfd) break;
	}
	MU_LOG_VERBOSE(logger, "Currently existing client?: %s\n", tmp_client ? "True" : "False");
	return tmp_client;
}

static int setup_bound_socket(NU_Bound_Socket_t *bsock, const char *ip_addr, unsigned int port, size_t queue_size){
	int i = 0, flag = 1;
	struct sockaddr_in my_addr;
	bsock->port = port;
	if((bsock->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		MU_LOG_BSOCK_ERR(socket, bsock);
		return 0;
	}
	if(setsockopt(bsock->sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
		shutdown(bsock->sockfd, SHUT_RDWR);
		MU_LOG_BSOCK_ERR(setsockopt, bsock);
		return 0;
	}	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(bsock->port);
	my_addr.sin_addr.s_addr = ip_addr ? inet_addr(ip_addr) : INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(bsock->sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		shutdown(bsock->sockfd, SHUT_RDWR);
		MU_LOG_BSOCK_ERR(bind, bsock);
		return 0;
	}
	if(listen(bsock->sockfd, queue_size) == -1){
		shutdown(bsock->sockfd, SHUT_RDWR);
		MU_LOG_BSOCK_ERR(listen, bsock);
		return 0;
	}
	return 1;
}

static NU_Bound_Socket_t *reuse_existing_socket(NU_Bound_Socket_t *head){
	NU_Bound_Socket_t *tmp_bsock = NULL;
	for(tmp_bsock = head; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(!tmp_bsock->sockfd) break;
	}
	MU_LOG_VERBOSE(logger, "Currently existing bsock?: %s\n", tmp_bsock ? "True" : "False");
	return tmp_bsock;
}

static void destroy_bound_socket(NU_Server_t *server, NU_Bound_Socket_t *bsock){
	shutdown(bsock->sockfd, SHUT_RDWR);
	// If a bound socket is being destroyed, then the list of bound sockets must be updated.
	NU_Bound_Socket_t *tmp_bsock = NULL;
	if(server->sockets == bsock){ 
		server->sockets = bsock->next;
		free(bsock);
		return;
	}
	for(tmp_bsock = server->sockets; tmp_bsock; tmp_bsock = tmp_bsock->next){
		if(tmp_bsock->next == bsock) {
			tmp_bsock->next = bsock->next;
			break;
		}
	}
	if(!tmp_bsock) MU_LOG_ERROR(logger, "The bsock to be deleted was not found in the list!\n");
	free(bsock);
}

NU_Server_t *NU_Server_create(){
	NU_Server_t *server = calloc(1, sizeof(NU_Server_t));
	if(!server) MU_LOG_ERROR(logger, "Was unable to allocate memory for server!\n");
	return server;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, const char *ip_addr, unsigned int port, size_t queue_size, int flags){
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
	if(!setup_bound_socket(bsock, ip_addr, port, queue_size)){
		MU_LOG_WARNING(logger, "bind->setup_bound_socket: \"was unable to setup bsock\"\n");
		return NULL;
	}
	server->amount_of_sockets++;
	return bsock;
}

int NU_Server_unbind(NU_Server_t *server, NU_Bound_Socket_t *bsock, const char *message){
	if(!server || !bsock || !bsock->sockfd) return 0;
	if(shutdown(bsock->sockfd, SHUT_RD) == -1){
		MU_LOG_BSOCK_ERR(shutdown, bsock);
		return 0;
	}
	NU_Client_Socket_t *client = NULL;
	for(client = server->clients; client; client = client->next){
		if(client->sockfd && client->port == bsock->port){
			NU_Server_disconnect(server, client, message);
		}
	}
	server->amount_of_sockets--;
	if(shutdown(bsock->sockfd, SHUT_RDWR) == -1){
		MU_LOG_BSOCK_ERR(shutdown, bsock);
		return 0;
	}
	return 1;
}

NU_Client_Socket_t *NU_Server_accept(NU_Server_t *server, NU_Bound_Socket_t *bsock, unsigned int timeout){
	int is_reused = 1;
	NU_Client_Socket_t *client = reuse_existing_client(server->clients);
	if(!client) {
		client = calloc(1, sizeof(NU_Client_Socket_t));
		MU_ASSERT_RETURN(client, logger, NULL, "Was unable to allocate memory for client!\n");
		is_reused--;
	}
	if(!is_reused){
		client->bbuf = calloc(1, sizeof(NU_Bounded_Buffer_t));
		if(!server->clients) server->clients = client;
		else {
			NU_Client_Socket_t *tmp_client = NULL;
			for(tmp_client = server->clients; tmp_client && tmp_client->next; tmp_client = tmp_client->next);
			tmp_client->next = client;
		}
	}
	char *ip_addr;
	if(!(client->sockfd = NUH_timed_accept(bsock->sockfd, &ip_addr, timeout, logger))){
		MU_LOG_INFO(logger, "accept->accept: \"timed out\"\n");
		return NULL;
	}
	strcpy(client->ip_addr, ip_addr);
	free(ip_addr);
	client->port = bsock->port;
	server->amount_of_clients++;
	MU_LOG_SERVER("%s connected to port %d\n", client->ip_addr, client->port);
	return client;
}

size_t NU_Server_send(NU_Server_t *server, NU_Client_Socket_t *client, const char *message, size_t msg_size, unsigned int timeout){
	if(!server || !client || !message || !msg_size) return 0;
	size_t result = NUH_send_all(client->sockfd, message, msg_size, timeout, logger);
	if(result != msg_size) MU_LOG_WARNING(logger, "Was unable to send all data to client!Total Sent: %zu, Message Size: %zu\n", result, msg_size);
	server->data.bytes_sent += result;
	server->data.messages_sent++;
	return result;
}

const char *NU_Server_receive(NU_Server_t *server, NU_Client_Socket_t *client, size_t buffer_size, unsigned int timeout){
	if(!server || !client || !buffer_size) return NULL;
	NUH_resize_buffer(client->bbuf, buffer_size+1, logger);
	size_t result = NUH_timed_receive(client->sockfd, client->bbuf, buffer_size, timeout, logger);
	MU_LOG_VERBOSE(logger, "Total received: %zu, Buffer Size: %zu\n", result, buffer_size);
	if(!result) return NULL;
	client->bbuf->buffer[result] = '\0';
	server->data.bytes_received += result;
	server->data.messages_received++;
	return (const char *)client->bbuf->buffer;
}

size_t NU_Server_receive_to_file(NU_Server_t *server, NU_Client_Socket_t *client, FILE *file, size_t buffer_size, unsigned int is_binary, unsigned int timeout){
	if(!server || !client || !client->sockfd || !file || !buffer_size) return 0;
	NUH_resize_buffer(client->bbuf, buffer_size, logger);
	size_t result, total_received = 0;
	while((result = NUH_timed_receive(client->sockfd, client->bbuf, buffer_size, timeout, logger)) == buffer_size){
		if(is_binary) fwrite(client->bbuf->buffer, 1, client->bbuf->size, file);
		else fprintf(file, "%.*s", (int)buffer_size, client->bbuf->buffer);
		total_received += result;
	}
	if(result){
		if(is_binary) fwrite(client->bbuf->buffer, 1, result, file);
		else fprintf(file, "%.*s", (int)result, client->bbuf->buffer);
		total_received += result;
	}
	server->data.bytes_received += total_received;
	server->data.messages_received++;
	return total_received;
}


size_t NU_Server_send_file(NU_Server_t *server, NU_Client_Socket_t *client, FILE *file, size_t buffer_size, unsigned int is_binary, unsigned int timeout){
	if(!server || !client || !client->sockfd || !file || !buffer_size) return 0;
	NUH_resize_buffer(client->bbuf, buffer_size, logger);
	size_t retval, total_sent = 0;
	char *str_retval;
	if(is_binary){
	  while((retval = fread(client->bbuf->buffer, 1, buffer_size, file)) == buffer_size){
	    if(NU_Server_send(server, client, client->bbuf->buffer, buffer_size, timeout) == 0){
		    MU_LOG_WARNING(logger, "server_send_file->server_send: \"%s\"\n", "Was unable to send all of message to client!\n");
		    return total_sent;
	    }
	    total_sent += retval;
	  } 
	} else {
	    while((str_retval = fgets(client->bbuf->buffer, buffer_size, file)) != NULL){
	      if(!NU_Server_send(server, client, client->bbuf->buffer, buffer_size, timeout)){
		MU_LOG_WARNING(logger, "server_send_file->server_send: \"%s\"\n", "Was unable to send all of message to server!\n");
		return total_sent;
	      }
	      total_sent += strlen(str_retval);
	    }
	}
	if(!total_sent) MU_LOG_WARNING(logger, "No data was sent to server!\n");
	else server->data.messages_sent++;
	server->data.bytes_sent += (size_t) total_sent;
	return (size_t) total_sent;
}



NU_Client_Socket_t **NU_Server_select_receive(NU_Server_t *server, NU_Client_Socket_t **clients, size_t *size, unsigned int timeout){
	if(!server || !clients || !size || !*size){
		*size = 0;
		return NULL;
	}
	fd_set receive_set;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	FD_ZERO(&receive_set);
	int max_fd = 0, retval;
	size_t i = 0, new_size = 0;
	for(;i < *size; i++){
		NU_Client_Socket_t *client = clients[i];
		if(!client || !client->sockfd) continue;
		FD_SET(client->sockfd, &receive_set);
		new_size++;
		if(client->sockfd > max_fd) max_fd = client->sockfd;
	}
	if(!new_size) {
		*size = 0;
		return NULL;
	}
	if((retval = TEMP_FAILURE_RETRY(select(max_fd + 1, &receive_set, NULL, NULL, &tv))) <= 0){
		if(!retval) MU_LOG_INFO(logger, "select_receive->select: \"timeout\"\n");
		else MU_LOG_WARNING(logger, "select_receive->select: \"%s\"\n", strerror(errno));
		*size = 0;
		return NULL;
	}
	NU_Client_Socket_t **ready_clients = malloc(sizeof(NU_Client_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(clients[i]->sockfd, &receive_set)) ready_clients[new_size++] = clients[i];
	*size = new_size;
	return ready_clients;
}

NU_Client_Socket_t **NU_Server_select_send(NU_Server_t *server, NU_Client_Socket_t **clients, size_t *size, unsigned int timeout){
	if(!server || !clients || !size || !*size){
		*size = 0;
		return NULL;
	}
	fd_set send_set;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	FD_ZERO(&send_set);
	int max_fd = 0, retval;
	size_t i = 0, new_size = 0;
	for(;i < *size; i++){
		NU_Client_Socket_t *client = clients[i];
		if(!client || !client->sockfd) continue;
		FD_SET(client->sockfd, &send_set);
		new_size++;
		if(client->sockfd > max_fd) max_fd = client->sockfd;
	}
	if(!new_size) {
		*size = 0;
		return NULL;
	}
	if((retval = TEMP_FAILURE_RETRY(select(max_fd + 1, NULL , &send_set, NULL, &tv))) <= 0){
		if(!retval) MU_LOG_VERBOSE(logger, "select_send->select: \"timeout\"\n");
		else MU_LOG_WARNING(logger, "select_send->select: \"%s\"\n", strerror(errno));
		*size = 0;
		return NULL;
	}
	NU_Client_Socket_t **ready_clients = malloc(sizeof(NU_Client_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(clients[i]->sockfd, &send_set)) ready_clients[new_size++] = clients[i];
	*size = new_size;
	return ready_clients;
}


char *NU_Server_about(NU_Server_t *server){
	char *about_server;
	char *bsock_str = bsock_to_string(server->sockets);
	char *data_str = NUH_data_to_string(server->data);
	char *client_str = clients_to_string(server->clients);
	asprintf(&about_server, "Bound to %zu ports: { %s }\nData usage: { %s }\n%zu clients connected: { %s }\n", server->amount_of_sockets, bsock_str, data_str, server->amount_of_clients, client_str);
	free(bsock_str);
	free(data_str);
	free(client_str);
	MU_LOG_INFO(logger, "About Server: \"%s\"\n", about_server);
	return about_server;
}

int NU_Server_log(NU_Server_t *server, const char *message, ...){
	if(!server || !message) return 0;
	va_list args;
	va_start(args, message);
	const buffer_size = 1024;
	char buffer[buffer_size];
	if(vsnprintf(buffer, buffer_size, message, args) < 0){ 
		MU_LOG_WARNING(logger, "log->vsnprintf: \"%s\"\n", strerror(errno));
		return 0;
	}
	MU_LOG_SERVER("%s", buffer);
	return 1;
}

int NU_Server_disconnect(NU_Server_t *server, NU_Client_Socket_t *client, const char *message){
	if(!server || !client) return 0;
	if(message){
		shutdown(client->sockfd, SHUT_RD);
		NU_Server_send(server, client, message, strlen(message), 0);
		shutdown(client->sockfd, SHUT_RDWR);
	} else shutdown(client->sockfd, SHUT_RDWR);
	MU_LOG_SERVER("%s disconnected from port %d\n", client->ip_addr, client->port);
	client->sockfd = 0;
	server->amount_of_clients--;
	return 1;
}

int NU_Server_shutdown(NU_Server_t *server, const char *message){
	if(!server) return 0;
	NU_Bound_Socket_t *bsock = NULL;
	for(bsock = server->sockets; bsock; bsock = bsock->next){
		NU_Server_unbind(server, bsock, message);
	}
	return server->amount_of_sockets == 0;
}

int NU_Server_destroy(NU_Server_t *server, const char *message){
	if(!server) return 0;
	if(server->amount_of_sockets) NU_Server_shutdown(server, message);
	delete_all_clients(server);
	delete_all_sockets(server);
	free(server);
	return 1;
}