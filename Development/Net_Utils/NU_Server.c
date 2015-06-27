#include <NU_Server.h>

MU_Logger_t *logger = NULL;

#define MU_LOG_BSOCK_ERR(function, bsock) do { \
	MU_LOG_VERBOSE(logger, "bsock: port->\"%d\", sockfd->%d, has_next: %s\n", bsock->port, bsock->sockfd, bsock->next ? "True" : "False"); \
	MU_LOG_WARNING(logger, #function ": \"%s\"\n", strerror(errno)); \
	bsock->sockfd = 0; \
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
	char *bsock_str = strdup("{ ");
	NU_Bound_Socket_t *bsock = NULL;
	for(bsock = head; bsock; bsock = bsock->next){
		char *old_str = bsock_str;
		asprintf(&bsock_str, "%s (port: %d, sockfd: %d),", bsock_str, bsock->port, bsock->sockfd);
		free(old_str);
	}
	return bsock_str;
}

static char *clients_to_string(NU_Client_Socket_t *head){
	if(!head) return NULL;
	char *clients_str = strdup("{ ");
	NU_Client_Socket_t *client = NULL;
	for(client = head; client; client = client->next){
		char *old_str = clients_str;
		asprintf(&client_str, "%s (sockfd: %d, ip_addr: %s, port: %d, bbuf: [init?: %s, size: %d])"
			clients_str, client->sockfd, client->ip_addr, client->port, client->bbuf ? "True" : "False",
			client->bbuf && client->bbuf->size ? client->bbuf->size : 0);
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

static int setup_bound_socket(NU_Bound_Socket_t *bsock, unsigned int port, size_t queue_size){
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
	my_addr.sin_addr.s_addr = INADDR_ANY;
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
	return server;
}

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, unsigned int port, size_t queue_size, int flags){
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
	if(!setup_bound_socket(bsock, port, queue_size)){
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
	if(!(client->sockfd = NUH_timed_accept(bsock->sockfd, &ip_addr, timeout))){
		MU_LOG_INFO(logger, "accept->accept: \"timed out\"\n");
		return NULL;
	}
	strcpy(client->ip_addr, ip_addr);
	free(ip_addr);
	client->port = bsock->port;
	server->amount_of_clients++;
	MU_LOG_SERVER(logger, "%s connected to port %d\n", client->ip_addr, client->port);
	return client;
}

int NU_Server_send(NU_Server_t *server, NU_Client_Socket_t *client, const char *message, unsigned int timeout){
	if(!server || !client || !message) return 0;
	size_t buffer_size = strlen(message);
	size_t result = NUH_send_all(client->sockfd, message, timeout);
	server->data.bytes_sent += result;
	server->data.messages_sent++;
	if(result != buffer_size) MU_LOG_WARNING(logger, "Was unable to send all data to client!Total Sent: %d, Message Size: %d\n", result, buffer_size);
	return result;
}

const char *NU_Server_receive(NU_Server_t *server, NU_Client_Socket_t *client, size_t buffer_size, unsigned int timeout){
	if(!server || !client || !buffer_size) return NULL;
	resize_buffer(client->bbuf, buffer_size);
	size_t result = timed_receive(client->sockfd, client->bbuf, timeout);
	MU_LOG_VERBOSE(logger, "Total received: %d, Buffer Size: %d\n", result, buffer_size);
	if(!result) return NULL;
	client->bbuf->buffer[result] = '\0';
	return (const char *)client->bbuf->buffer;
}

size_t NU_Server_receive_to_file(NU_Server_t *server, NU_Client_Socket_t *client, FILE *file, size_t buffer_size, unsigned int timeout, int flags){
	if(!server || !client || !client->sockfd || !file || !buffer_size) return 0;
	resize_buffer(client->bbuf, buffer_size);
	size_t result, total_received = 0;
	int binary_read = NUH_is_selected(flags, NU_BINARY);
	while((result = NUH_timed_receive(client->sockfd, client->bbuf, timeout)) == buffer_size){
		if(binary_read) fwrite(client->bbuf->buffer, 1, client->bbuf->size, file);
		else fprintf(file, "%.*s", buffer_size, client->bbuf->buffer);
		total_received += result;
	}
	if(result){
		if(binary_read) fwrite(client->bbuf->buffer, 1, result, file);
		else fprintf(file, "%.*s", result, client->bbuf->buffer);
		total_received += result;
	}
	server->data.bytes_received += total_received;
	server->data.messages_received++;
	return total_received;

}


size_t NU_Server_send_file(NU_Server_t *server, NU_Client_Socket_t *client, FILE *file, size_t buffer_size, unsigned int timeout){
	if(!server || !client || !client->sockfd || !file || !buffer_size) return 0;
	int file_fd;
	if((file_fd = fileno(file)) == -1){
		MU_LOG_WARNING(logger, "send_file->fileno: \"%s\"\n", strerror(errno));
		return 0;
	}
	struct stat get_size;
	size_t file_size;
	if(fstat(file_fd, &get_size) == -1){
		MU_LOG_WARNING(logger, "send_file->fstat: \"%s\"\n", strerror(errno));
		return 0;
	}
	file_size = get_size.st_size;
	MU_LOG_VERBOSE(logger, "Passed File Size is %u\n", file_size);
	ssize_t retval;
	if((retval = sendfile(client->sockfd, fileno(file), NULL, file_size)) == -1){
		MU_LOG_WARNING(logger, "send_file->sendfile: \"%s\"\n", strerror(errno));
		return 0;
	}
	return (size_t) retval;
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
		if(client->sockfd > max_fd) max_fd = client->sockfd);
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
	for(i = 0;i < *size;i++) if(FD_ISSET(clients[i]->sockfd, &can_receive)) ready_clients[new_size++] = clients;
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
		if(client->sockfd > max_fd) max_fd = client->sockfd);
	}
	if(!new_size) {
		*size = 0;
		return NULL;
	}
	if((retval = TEMP_FAILURE_RETRY(select(max_fd + 1, NULL , &send_set, NULL, &tv))) <= 0){
		if(!retval) MU_LOG_VERBOSE(logger, "select_send->select: \"timeout\"\n", timeout);
		else MU_LOG_WARNING(logger, "select_send->select: \"%s\"\n", strerror(errno));
		*size = 0;
		return NULL;
	}
	NU_Client_Socket_t **ready_clients = malloc(sizeof(NU_Client_Socket_t *) * retval);
	new_size = 0;
	for(i = 0;i < *size;i++) if(FD_ISSET(clients[i]->sockfd, &send_set)) ready_clients[new_size++] = clients;
	*size = new_size;
	return ready_clients;
}


char *NU_Server_about(NU_Server_t *server){
	char *about_server;
	char *bsock_str = bsock_to_string(server->sockets);
	char *data_str = NUH_data_to_string(server->data);
	char *client_str = clients_to_string(server->clients);
	asprintf(&about_server, "Bound to %d ports: %s\nData usage: %s\n%d clients connected: %s\n", bsock_str, data_str, client_str);
	free(bsock_string);
	free(data_str);
	free(client_str);
	MU_LOG_INFO(logger, "About Server: \"%s\"\n", about_server);
}

int NU_Server_disconnect(NU_Server_t *server, NU_Client_Socket_t *client, const char *message){
	if(!server || !client) return 0;
	if(message){
		shutdown(client->sockfd, SHUT_RD);
		NU_Server_send(server, client, message, 0);
		shutdown(client->sockfd, SHUT_RDWR);
	} else shutdown(client->sockfd, SHUT_RDWR);
	MU_LOG_SERVER(logger, "%s disconnected from port %d\n", client->ip_addr, client->port);
	client->sockfd = 0;
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