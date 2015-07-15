#include <NU_Connection.h>

// Returns the max sockfd size.
static int add_valid_connections_to_fd_set(NU_Connection_t **connections, size_t size, fd_set *set, MU_Logger_t *logger){
	if(connections) return -1;
	size_t i = 0, max_fd = -1;
	for(;i < size; i++){
		NU_Connection_t *conn = connections[i];
		MU_Cond_rwlock_rdlock(conn->lock, logger);
		if(!conn->in_use){
			MU_Cond_rwlock_unlock(conn->lock, logger);
			continue;
		}
		int sockfd = conn->sockfd;
		FD_SET(sockfd, set);
		if(sockfd > max_fd){
			max_fd = sockfd;
		}
		MU_Cond_rwlock_unlock(conn->lock, logger);
	}
	return max_fd;
}

char *NU_Connection_Type_to_string(NU_Connection_Type_t type){
	switch(type){
	case NU_CLIENT: return "Client";
	case NU_SERVER: return "Server";
	case NU_HTTP: return "HTTP";
	case NU_OTHER: return "Unknown";
	default: return NULL;
	}
} 

// Implement
NU_Connection_t *NU_Connection_create(NU_Connection_Type_t type, unsigned char init_locks, MU_Logger_t *logger){
	NU_Connection_t *conn = calloc(1, sizeof(NU_Connection_t));
	if(!conn){
		MU_LOG_ASSERT(logger, "NU_Connection_create->calloc: \"%s\"\n", strerror(errno));
		return NULL;
	}
	conn->type = type;
	if(init_locks){
		conn->lock = malloc(sizeof(pthread_rwlock_t));
		if(!conn->lock){
			free(conn);
			MU_LOG_ASSERT(logger, "NU_Connection_create->malloc: \"%s\"\n", strerror(errno));
			return NULL;
		}
		int retval;
		if((retval = pthread_rwlock_init(conn->lock, NULL)) < 0){
			free(conn->lock);
			free(conn);
			MU_LOG_ERROR(logger, "NU_Connection_create->pthread_rwlock_init: \"%s\"\n", strerror(retval));
			return NULL;
		}
	}
	return conn;
}

// Implement
size_t NU_Connection_send(NU_Connection_t *conn, const void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !buffer || !buf_size){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Connection: %s;Buffer: %s;Buffer Size > 0: %s\"\n",
				conn ? "OK!" : "NULL", buffer ? "OK!" : "NULL", buf_size ? "OK!" : "NO!");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	size_t total_sent = NU_send_all(conn->sockfd, buffer, buf_size, timeout, logger);
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return total_sent;
}

// Implement
size_t NU_Connect_receive(NU_Connection_t *conn, void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !buffer || !buf_size){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Connection: %s;Buffer: %s;Buffer Size > 0: %s\"\n",
				conn ? "OK!" : "NULL", buffer ? "OK!" : "NULL", buf_size ? "OK!" : "NO!");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	size_t amount_received = NU_timed_receive(conn->sockfd, buffer, buf_size, timeout, logger);
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return amount_received;
}

// Implement
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !file || !buf_size){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Connection: %s;File: %s;Buffer Size > 0: %s\"\n",
				conn ? "OK!" : "NULL", file ? "OK!" : "NULL", buf_size ? "OK!" : "NO!");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	char buf[buf_size];
	size_t buf_read, total_sent = 0;
	while((buf_read = TEMP_FAILURE_RETRY(fread(buf, 1, buf_size, file))) > 0){
		if(NU_send_all(conn->sockfd, buf, buf_read, timeout, logger) != buf_read){
			MU_LOG_WARNING(logger, "NU_Connection_send_file->NU_send_all: \"Was unable to send all of message to %s\"\n", conn->ip_addr);
			MU_Cond_rwlock_unlock(conn->lock, logger);
			return total_sent;
		}
		total_sent += buf_read;
	}
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return total_sent;
}

// Implement
size_t NU_Connection_receive_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !file || !buf_size){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Connection: %s;File: %s;Buffer Size > 0: %s\"\n",
				conn ? "OK!" : "NULL", file ? "OK!" : "NULL", buf_size ? "OK!" : "NO!");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	char buf[buf_size];
	size_t received, total_received = 0;
	while((received = NU_timed_receive(conn->sockfd, buf, buf_size, timeout, logger)) > 0){
		size_t written = TEMP_FAILURE_RETRY(fwrite(buf, 1, received, file));
		if(written != received){
			MU_LOG_ERROR(logger, "NU_Connection_receive_file->fwrite: \"Written only %zu bytes, expected %zu bytes!\n%s\"\n", written, received, strerror(errno));
			return total_received += written;
		}
		total_received += received;
	}
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return total_received;
}

int NU_Connection_select(NU_Connection_t ***receivers, size_t *r_size, NU_Connection_t ***senders, size_t *s_size, unsigned int timeout, MU_Logger_t *logger){
	NU_Connection_t **send_connections = NULL;
	NU_Connection_t **recv_connections = NULL;
	// Initialized at top for goto statement consistency.
	if((!receivers || !r_size || !*r_size) && (!senders || !s_size || *s_size)){
		MU_LOG_ERROR(logger, "Invalid Arguments: \"Receivers: %s;Receiver Size_ptr: %s;Receiver Size > 0: %s;\n"
				"Senders: %s;Sender Size_ptr: %s;Sender Size > 0: %s\"\nMessage: \"%s\"\n", receivers ? "OK!" : "NULL",
						r_size ? "OK!" : "NO!", *r_size ? "OK!" : "NO!", senders ? "OK!" : "NULL", s_size ? "OK!" : "NULL",
								*s_size ? "OK!" : "NO!", "Neither receivers nor senders were valid!");
		goto error;
	}
	fd_set receive_set;
	fd_set send_set;
	NU_Connection_t **r_conns = receivers ? *receivers : NULL;
	NU_Connection_t **s_conns = senders ? *senders : NULL;
	size_t recv_size = r_size ? *r_size : 0;
	size_t send_size = s_size ? *s_size : 0;
	struct timeval tv = { .tv_sec = timeout };
	FD_ZERO(&receive_set);
	FD_ZERO(&send_set);
	int max_fd = 0, can_receive = 0, can_send = 0;
	int r_max_fd = add_valid_connections_to_fd_set(r_conns, recv_size, &receive_set, logger);
	can_receive = (r_max_fd != -1);
	if(r_max_fd > max_fd){
		max_fd = r_max_fd;
	}
	int s_max_fd = add_valid_connections_to_fd_set(s_conns, send_size, &send_set, logger);
	can_send = (s_max_fd != -1);
	if(s_max_fd > max_fd){
		max_fd = s_max_fd;
	}
	if(!can_receive && !can_send){
		MU_LOG_WARNING(logger, "NU_Connection_select: \"Was unable to find a valid receiver or sender connection!\"\n");
		goto error;
	}
	size_t are_ready;
	if((are_ready = TEMP_FAILURE_RETRY(select(max_fd + 1, &receive_set, &send_set, NULL, &tv))) <= 0){
		if(!are_ready) MU_LOG_INFO(logger, "NU_Connection_select->select: \"Timed out!\"\n");
		else MU_LOG_WARNING(logger, "NU_Connection_select->select: \"%s\"\n", strerror(errno));
		goto error;
	}
	recv_connections = malloc(sizeof(NU_Connection_t *) * are_ready);
	if(!recv_connections){
		MU_LOG_ASSERT(logger, "NU_Connection_select->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	send_connections = malloc(sizeof(NU_Connection_t *) * are_ready);
	if(!send_connections){
		MU_LOG_ASSERT(logger, "NU_Connection_select->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	size_t i = 0;
	for(; i < recv_size && r_conns; i++){
		NU_Connection_t *conn = r_conns[i];
		if(FD_ISSET(conn->sockfd, &receive_set)){
			recv_connections[can_receive++] = conn;
		}
	}
	NU_Connection_t **tmp_recv_connections = realloc(recv_connections, sizeof(NU_Connection_t *) * can_receive);
	if(can_receive && !tmp_recv_connections){
		MU_LOG_ASSERT(logger, "NU_Connection_select->realloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	recv_connections = tmp_recv_connections;
	for(i = 0; i < send_size && s_conns; i++){
		NU_Connection_t *conn = s_conns[i];
		if(FD_ISSET(conn->sockfd, &send_set)){
			send_connections[can_send++] = conn;
		}
	}
	NU_Connection_t **tmp_send_connections = realloc(send_connections, sizeof(NU_Connection_t *) * can_send);
	if(can_send && !tmp_send_connections){
		MU_LOG_ASSERT(logger, "NU_Connection_select->realloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	*receivers = recv_connections;
	*senders = send_connections;
	*r_size = can_receive;
	*s_size = can_send;
	return are_ready;

	error:
		// TODO: Test this! Frees them before even declared.
		free(send_connections);
		free(recv_connections);
		*receivers = NULL;
		*r_size = 0;
		*senders = NULL;
		*s_size = 0;
		return 0;
}

NU_Connection_t *NU_Connection_reuse(NU_Connection_t **connections, size_t size, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger){
	if(!connections || !size || sockfd < 0 || !port || !ip_addr){
		MU_LOG_ERROR(logger, "NU_Connection_reuse: Invalid Argument=> \"Connection: %s;Size > 0: %s;Sockfd >= 0: %s;Port > 0: %s;IP Address: %s\"\n",
		connections ? "OK!" : "NULL", size ? "OK!" : "NO!", sockfd >= 0 ? "OK!" : "NO!", port ? "OK!" : "NO!", ip_addr ? "OK!" : "NULL");
		return NULL;
	}
	size_t i = 0;
	for(;i < size; i++){
		NU_Connection_t *conn = connections[i];
		MU_Cond_rwlock_wrlock(conn->lock, logger);
		if(conn && !conn->in_use){
			conn->in_use = 1;
			conn->sockfd = sockfd;
			conn->port = port;
			strncpy(conn->ip_addr, ip_addr, INET_ADDRSTRLEN);
			NU_unlock_rwlock(conn->lock, logger);
			return conn;
		}
		MU_Cond_rwlock_unlock(conn->lock, logger);
	}
	return NULL;
}

// Implement
char *NU_Connection_to_string(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_to_string: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return NULL;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	char *conn_str;
	asprintf(&conn_str, "(sockfd: %d, port: %u, ip_addr: %s, type: %s, has_lock: %s, in_use: %s)",
			conn->sockfd, conn->port, conn->ip_addr, NU_Connection_Type_to_string(conn->type), conn->lock ? "True" : "False", conn->in_use ? "True" : "False");
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return conn_str;
}

int NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd, MU_Logger_t *logger){
	if(!conn || sockfd < 0){
		MU_LOG_ERROR(logger, "NU_Connection_set_sockfd: Invalid Arguments=> \"Connection: %s;Sockfd >= 0: %s\"\n", conn ? "OK!" : "NULL", sockfd >= 0 ? "OK!" : "NO!");
		return 0;
	}
	MU_Cond_rwlock_wrlock(conn->lock, logger);
	conn->sockfd = sockfd;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return 1;
}

int NU_Connection_get_sockfd(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_get_sockfd: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return -1;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	int sockfd = conn->sockfd;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return sockfd;
}

int NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr, MU_Logger_t *logger){
	if(!conn || !ip_addr){
		MU_LOG_ERROR(logger, "NU_Connection_set_ip_addr: Invalid Arguments=> \"Connection: %s;IP Address: %s\"\n", conn ? "OK!" : "NULL", ip_addr ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_wrlock(conn->lock, logger);
	strncpy(conn->ip_addr, ip_addr, INET_ADDRSTRLEN);
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return 1;
}

const char *NU_Connection_get_ip_addr(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_get_ip_addr: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return NULL;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	const char *ip_addr  = conn->ip_addr;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return ip_addr;
}

unsigned int NU_Connection_get_port(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_get_port: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	unsigned int port  = conn->port;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return port;
}

int NU_Connection_set_port(NU_Connection_t *conn, unsigned int port, MU_Logger_t *logger){
	if(!conn || !port){
		MU_LOG_ERROR(logger, "NU_Connection_set_port: Invalid Argument=> \"Connection: %s;Port > 0: %s\"\n", conn ? "OK!" : "NULL", port ? "OK!" : "NO!");
		return 0;
	}
	MU_Cond_rwlock_wrlock(conn->lock, logger);
	conn->port = port;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return 1;
}

int NU_Connection_init(NU_Connection_t *conn, int sockfd, unsigned int port, const char *ip_addr, MU_Logger_t *logger){
	if(!conn || !ip_addr || sockfd < 0 || !port){
		MU_LOG_ERROR(logger, "NU_Connection_init: Invalid Arguments=> \"Connection: %s;Sockfd >= 0: %s;Port > 0: %s; IP Address: %s\"\n",
				conn ? "OK!" : "NULL", sockfd >= 0 ? "OK!" : "NO!", port ? "OK!" : "NO!", ip_addr ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_wrlock(conn->lock, logger);
	if(conn->in_use){
		MU_LOG_INFO(logger, "NU_Connection_init: \"Connection already in use!\"\n");
		MU_Cond_rwlock_unlock(conn->lock, logger);
		return 0;
	}
	conn->sockfd = sockfd;
	conn->port = port;
	strcpy(conn->ip_addr, ip_addr);
	conn->in_use = 1;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return 1;
}

int NU_Connection_is_valid(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_get_port: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	int result = 1;
	if(!conn->in_use){
		result--;
	}
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return result;
}

int NU_Connection_in_use(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_get_port: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_rdlock(conn->lock, logger);
	int in_use = conn->in_use;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return in_use;
}

int NU_Connection_disconnect(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn){
		MU_LOG_ERROR(logger, "NU_Connection_get_port: Invalid Argument=> \"Connection: %s\"\n", conn ? "OK!" : "NULL");
		return 0;
	}
	MU_Cond_rwlock_wrlock(conn->lock, logger);
	if(!conn->in_use){
		MU_Cond_rwlock_unlock(conn->lock, logger);
		return 0;
	}
	if(TEMP_FAILURE_RETRY(close(conn->sockfd)) == -1) MU_LOG_WARNING(logger, "NU_Connection_disconnect->close: \"%s\"\n", strerror(errno));
	conn->in_use = 0;
	MU_Cond_rwlock_unlock(conn->lock, logger);
	return 1;
}

int NU_Connection_destroy(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn) return 0;
	NU_Connection_disconnect(conn, logger);
	MU_Cond_rwlock_destroy(conn->lock, logger);
	free(conn);
	return 1;
}
