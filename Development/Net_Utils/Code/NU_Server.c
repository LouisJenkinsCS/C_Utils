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

static NU_Bound_Socket_t *create_server_socket(char *port){
	NU_Bound_Socket_t *bsock = calloc(1, sizeof(NU_Bound_Socket_t));
	MU_ASSERT_RETURN(logger, bsock, NULL, "Was unable to allocate memory for bound socket!");
	int i = 0, flag = 1;
	while(bsock->port[i] = port[i++]);
	bsock->port[5] = '\0';
	if((bsock->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		MU_LOG_WARNING(logger, "socket: \"%s\"\n", strerror(-1));
		free(bsock);
		return NULL;
	}
	if(setsockopt(bsock->sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
		MU_LOG_WARNING(logger, "setsockopt: \"%s\"\n", strerror(-1));
		shutdown(bsock->sockfd, 2);
		free(bsock);
		return NULL;
	}
	return bsock;
}

static void destroy_bound_socket(NU_Bound_Socket_t *head_bsock, NU_Bound_Socket_t *current_bsock){
	shutdown(current_bsock->sockfd, 2);
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

NU_Bound_Socket_t *NU_Server_bind(NU_Server_t *server, char *port, int flags){
	if(!server || !port) return NULL;
	int retval;
	struct sockaddr_in my_addr;
	NU_Bound_Socket_t *bsock = create_server_socket(port);
	if(!bsock) return NULL;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(bsock->port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if(bind(bsock->sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
		MU_LOG_VERBOSE(logger, "bsock: port->\"%s\", sockfd->%d", bsock->port, bsock->sockfd);
		MU_LOG_WARNING(logger, "bind: \"%s\"\n", strerror(-1));
		shutdown(bsock, 2);
		free(bsock);
		return NULL;
	}
	// Now add the bsock to the list of sockets.
	if(!server->sockets){
		server->sockets = bsock;
		return bsock;
	}
	NU_Bound_Socket_t *tmp_bsock = NULL;
	for(tmp_bsock = server->sockets; tmp_bsock && tmp_bsock->next; tmp_bsock = tmp_bsock->next);
	tmp_bsock->next = bsock;
	return bsock;
}
