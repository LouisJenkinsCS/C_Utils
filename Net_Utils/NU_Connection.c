// Implement
NU_Connection_t *NU_Connection_create(NU_Connection_Type_t type, unsigned char init_locks, MU_Logger_t *logger){
	NU_Connection_t *conn = calloc(1, sizeof(NU_Connect_t));
	MU_ASSERT_RETURN(conn, logger, NULL, "NU_Connection_create->calloc: \"%s\"\n", strerror(errno));
	conn->type = type;
	conn->buf = NU_Buffer_create(0, init_locks, logger);
	// Because MU_ASSERT_RETURN has to return the value, that leaves no room to deallocate; So we deallocate first then pass a failing condition: "NULL"
	if(!conn->buf){
		free(conn);
		MU_ASSERT_RETURN(NULL, logger, NULL, "NU_Connection_create->NU_Buffer_create: \"Was unable to create buffer!\"\n");
	}
	if(init_locks){
		conn->lock = malloc(sizeof(pthread_rwlock_t));
		if(!lock){
			NU_Buffer_destroy(conn->buf);
			free(conn);
			MU_ASSERT_RETURN(NULL, logger, NULL, "NU_Connection_create->malloc: \"%s\"\n", strerror(errno));
		}
		int retval;
		if((retval = pthread_rwlock_init(conn->lock, NULL)) < 0){
			NU_Buffer_destroy(conn->buf);
			free(conn);
			MU_LOG_ERROR(logger, "NU_Connection_create->pthread_rwlock_init: \"%s\"\n", strerror(retval));
			return NULL;
		}
	}
	return conn;
}

// Implement
size_t NU_Connection_send(NU_Connect_t *conn, const void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !conn->sockfd || !buffer || !buf_size) return 0;
	// Lock and Unlock 
	return NU_send_all(conn->sockfd, buffer, buf_size, timeout, logger);
}

// Implement
void *NU_Connect_receive(NU_Connect_t *conn, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !conn->sockfd || !buf_size) return NULL;
	NU_Buffer_resize(conn->buf, buf_size, logger);
	// Lock and Unlock
	return NU_timed_receive(conn->sockfd, conn->bbuf, buf_size, timeout, logger);
}

// Implement
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || || !conn->sockfd || !file || !buf_size) return 0;
	NU_Buffer_resize(conn->buf, buf_size, logger);
	size_t buf_read, total_sent = 0;
	while((buf_read = fread(conn->buf->buffer, 1, buf_size, file)) > 0){
		if(NU_Connection_send(conn, conn->buf->buffer, buf_read, timeout, logger) == 0){
			MU_LOG_WARNING(logger, "NU_Connection_send_file->NU_Connection_send: \"Was unable to send all of message to %s\"\n", conn->ip_addr);
			return total_sent;
		}
		total_sent += retval;
	}
	return total_sent;
}

// Implement
size_t NU_Connection_receive_file(NU_Connect_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !file || !buf_size) return 0;
	NU_Buffer_resize(conn->bbuf, buf_size, logger);
	size_t result, total_received = 0;
	while((result = NU_timed_receive(conn->sockfd, conn->bbuf, buf_size, timeout, logger)) > 0){
		fwrite(client->bbuf->buffer, 1, result, file);
		total_received += result;
	}
	return total_received;
}

// Implement
char *NU_Connection_to_string(NU_Connection_t *connection);

void NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd);

int NU_Connection_get_sockfd(NU_Connection_t *conn);

const char *NU_Connection_get_ip_addr(NU_Connection_t *conn);

void NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr);

unsigned int NU_Connection_get_port(NU_Connection_t *conn);

void NU_Connection_set_port(NU_Connection_t *conn, unsigned int port);

void NU_Connection_disconnect(NU_Connection_t *conn);

void NU_Connection_destroy(NU_Connection_t *conn);