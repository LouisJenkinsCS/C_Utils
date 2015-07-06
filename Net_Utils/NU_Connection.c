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
	if(!conn || !buffer | !buf_size) return 0;
	NU_lock_rdlock(conn->lock);
	if(!conn->sockfd){
		NU_unlock_rdlock(conn->lock);
		return 0;
	}
	size_t total_sent = NU_send_all(conn->sockfd, buffer, buf_size, timeout, logger);
	NU_unlock_rdlock(conn->lock);
	return total_sent;
}

// Implement
size_t NU_Connect_receive(NU_Connect_t *conn, void *buffer, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !buf_size) return NULL;
	NU_lock_rdlock(conn->lock)
	if(!conn->sockfd){
		NU_unlock_rdlock(conn->lock);
		MU_LOG_INFO(logger, "NU_Connection_send_file: \"Connection contained bad sockfd!\"\n");
		*buf_size = 0;
		return NULL;
	}
	size_t amount_received = NU_timed_receive(conn->sockfd, buffer, buf_size, timeout, logger);
	NU_unlock_rdlock(conn->lock);
	return amount_received;
}

// Implement
size_t NU_Connection_send_file(NU_Connection_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !file || !buf_size) return 0;
	NU_lock_rdlock(conn->lock);
	if(!conn->sockfd){
		NU_unlock_rdlock(conn->lock);
		MU_LOG_INFO(logger, "NU_Connection_send_file: \"Connection contained bad sockfd!\"\n");
		return 0;
	}
	char buf[buf_size];
	size_t buf_read, total_sent = 0;
	while((buf_read = TEMP_FAILURE_RETRY(fread(buf, 1, buf_size, file))) > 0){
		if(NU_send_all(conn, buf, buf_read, timeout, logger) == 0){
			MU_LOG_WARNING(logger, "NU_Connection_send_file->NU_send_all: \"Was unable to send all of message to %s\"\n", conn->ip_addr);
			NU_unlock_rdlock(conn->lock);
			return total_sent;
		}
		total_sent += buf_read;
	}
	NU_unlock_rdlock(conn->lock);
	return total_sent;
}

// Implement
size_t NU_Connection_receive_file(NU_Connect_t *conn, FILE *file, size_t buf_size, unsigned int timeout, MU_Logger_t *logger){
	if(!conn || !file || !buf_size) return 0;
	NU_lock_rdlock(conn->lock);
	if(!conn->sockfd){
		NU_unlock_rdlock(conn->lock);
		MU_LOG_INFO(logger, "NU_Connection_receive_file: \"Connection contained bad sockfd!\"\n");
		return 0;
	}
	char buf[buf_size];
	size_t result, total_received = 0;
	while((result = NU_timed_receive(conn->sockfd, buf, buf_size, timeout, logger)) > 0){
		size_t written = 0;
		if((written = TEMP_FAILURE_RETRY(fwrite(buf, 1, result, file))) != result){
			MU_LOG_ERROR(logger, "NU_Connection_receive_file->fwrite: \"Written only %zu bytes, expected %zu bytes!\n%s\"\n", written, result, strerror(errno));
			return total_received += written;
		}
		total_received += result;
	}
	NU_unlock_rdlock(conn->lock);
	return total_received;
}

// Implement
char *NU_Connection_to_string(NU_Connection_t *connection);

void NU_Connection_set_sockfd(NU_Connection_t *conn, int sockfd){
	if(!conn || sockfd < 0) return;
	NU_lock_wrlock(conn->lock);
	conn->sockfd = sockfd;
	NU_unlock_wrlock(conn->lock);
}

int NU_Connection_get_sockfd(NU_Connection_t *conn){
	if(!conn) return -1;
	NU_lock_rdlock(conn->lock);
	int sockfd = conn->sockfd;
	NU_unlock_rdlock(conn->lock);
	return sockfd;
}

void NU_Connection_set_ip_addr(NU_Connection_t *conn, const char *ip_addr){
	if(!conn || !ip_addr) return;
	NU_lock_wrlock(conn->lock);
	conn->ip_addr = ip_addr;
	NU_unlock_wrlock(conn->lock);
}

const char *NU_Connection_get_ip_addr(NU_Connection_t *conn){
	if(!conn) return NULL;
	NU_lock_rdlock(conn->lock);
	const char *ip_addr  = conn->ip_addr;
	NU_unlock_rdlock(conn->lock);
	return ip_addr;
}

unsigned int NU_Connection_get_port(NU_Connection_t *conn){
	if(!conn) return 0;
	NU_lock_rdlock(conn->lock);
	unsigned int port  = conn->port;
	NU_unlock_rdlock(conn->lock);
	return port;
}

void NU_Connection_set_port(NU_Connection_t *conn, unsigned int port){
	if(!conn || !port) return;
	NU_lock_wrlock(conn->lock);
	conn->port = port;
	NU_unlock_wrlock(conn->lock);
}

void NU_Connection_disconnect(NU_Connection_t *conn, MU_Logger_t *logger){
	if(!conn) return;
	NU_lock_wrlock(conn->lock);
	if(!conn->sockfd){
		NU_unlock_wrlock(conn->lock);
		return;
	}
	if(TEMP_FAILURE_RETRY(close(conn->sockfd)) == -1){
		MU_LOG_WARNING(logger, "NU_Connection_disconnect->close: \"%s\"\n", strerror(errno));

	}
}

void NU_Connection_destroy(NU_Connection_t *conn);