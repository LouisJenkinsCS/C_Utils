#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "networking/connection.h"
#include "misc/argument_check.h"
#include "misc/signal_retry.h"

struct c_utils_connection {
	/// Socket file descriptor associated with host.
   volatile int sockfd;
   /// The IP Address of the host connected to.
   char ip_addr[INET_ADDRSTRLEN];
   /// Port number that the host is bound to.
   unsigned int port;
   /// Read-Write lock to use for synchronization if initialized.
   struct c_utils_scoped_lock *lock;
   /// A reusable buffer for each connection.
   volatile bool in_use;
   /// Logger associated with each connection.
   struct c_utils_logger *logger;
};

static const int send_buf_size = 8 * 1024;

static const int send_flags = MSG_NOSIGNAL;

static size_t timed_receive(int sockfd, void *buffer, size_t buf_size, long long int timeout, int flags, struct c_utils_logger *logger) {
   long long int received;
   struct timeval tv;
   fd_set can_receive;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;

   FD_ZERO(&can_receive);
   FD_SET(sockfd, &can_receive);
   
   C_UTILS_TEMP_FAILURE_RETRY(received, select(sockfd + 1, &can_receive, NULL, NULL, timeout < 0 ? NULL : &tv));
   if (received <= 0) {
      if (!received) C_UTILS_LOG_INFO(logger, "select: 'Timed out!'");
      else C_UTILS_LOG_ERROR(logger, "select: '%s'", strerror(errno));
      return 0;
   }

   C_UTILS_TEMP_FAILURE_RETRY(received, recv(sockfd, buffer, buf_size, flags));
   if (received <= 0) {
      if (!received) C_UTILS_LOG_INFO(logger, "recv: 'Disconnected from the stream!'");
      else C_UTILS_LOG_ERROR(logger, "recv: '%s'", strerror(errno));
      return 0;
   }

   return received;
}

static size_t send_all(int sockfd, const void *buffer, size_t buf_size, long long int timeout, int flags, struct c_utils_logger *logger) {
   size_t total_sent = 0, data_left = buf_size;
   long long int sent;
   struct timeval tv;
   fd_set can_send, can_send_copy;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   
   FD_ZERO(&can_send);
   FD_SET(sockfd, &can_send);
   
   while (buf_size > total_sent) {
      can_send_copy = can_send;
      // Restart timeout.
      tv.tv_sec = timeout;
      
      C_UTILS_TEMP_FAILURE_RETRY(sent, select(sockfd+1, NULL, &can_send_copy, NULL, timeout < 0 ? NULL : &tv));
      if (sent <= 0) {
         if (!sent) C_UTILS_LOG_INFO(logger, "select: 'Timed out!'");
         else C_UTILS_LOG_ERROR(logger, "select: '%s'", strerror(errno));
         break;
      }

      C_UTILS_TEMP_FAILURE_RETRY(sent, send(sockfd, buffer + total_sent, data_left, flags | send_flags));
      if (sent <= 0) {
         if (!sent) C_UTILS_LOG_INFO(logger, "send: 'Disconnected from the stream'");
         else C_UTILS_LOG_ERROR(logger, "send: '%s'", strerror(errno));
         break;
      }

      total_sent += sent;
      data_left -= sent;
   }
   return total_sent;
}

// Returns the max sockfd size.
static int add_valid_connections_to_fd_set(struct c_utils_connection **connections, size_t size, fd_set *set) {
	if (!connections) return -1;
	
	size_t i = 0, max_fd = -1;
	for (;i < size; i++) {
		struct c_utils_connection *conn = connections[i];

		// Acquire Reader Lock
		SCOPED_LOCK1(conn->lock) {
			if (!conn->in_use)  
				continue;
			
			int sockfd = conn->sockfd;
			FD_SET(sockfd, set);
			if (sockfd > max_fd)  
				max_fd = sockfd;
			
		} // Release Reader Lock
	}
	return max_fd;
}

// Implement
struct c_utils_connection *c_utils_connection_create(bool init_locks, struct c_utils_logger *logger) {
	struct c_utils_connection *conn = calloc(1, sizeof(struct c_utils_connection));
	if (!conn) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}

	bool rwlock_init = false;
	pthread_rwlock_t *lock = NULL;

	if (init_locks) {
		lock = malloc(sizeof(pthread_rwlock_t));
		if (!conn->lock) {
			C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
			return NULL;
		}

		int retval;
		if ((retval = pthread_rwlock_init(lock, NULL)) < 0) {
			C_UTILS_LOG_ERROR(logger, "pthread_rwlock_init: '%s'", strerror(retval));
			goto error;
			return NULL;
		}
		
		rwlock_init = true;
	}

	conn->lock = SCOPED_LOCK_FROM(lock, logger);
	if (!conn->lock) {
		C_UTILS_LOG_ERROR(logger, "SCOPED_LOCK_FROM: 'Unable to create scoped lock from rwlock!");
		goto error;
	}

	conn->logger = logger;
	return conn;

	error:
		if (conn) {
			if (conn->lock)   
				c_utils_scoped_lock_destroy(conn->lock);
			 else if (lock) {
				if (rwlock_init)   
					pthread_rwlock_destroy(lock);
				
				free(lock);
			}
		}
		free(conn);
		return NULL;
}

// Implement
size_t c_utils_connection_send(struct c_utils_connection *conn, const void *buffer, size_t buf_size, long long int timeout, int flags) {
	C_UTILS_ARG_CHECK(conn->logger, 0, conn, buffer, buf_size > 0);
	
	SCOPED_LOCK1(conn->lock) return send_all(conn->sockfd, buffer, buf_size, timeout, flags, conn->logger);
}

// Implement
size_t c_utils_connection_receive(struct c_utils_connection *conn, void *buffer, size_t buf_size, long long int timeout, int flags) {
	C_UTILS_ARG_CHECK(conn->logger, 0, conn, buffer, buf_size > 0);

	SCOPED_LOCK1(conn->lock) return timed_receive(conn->sockfd, buffer, buf_size, timeout, flags, conn->logger);
}

// Implement
size_t c_utils_connection_send_file(struct c_utils_connection *conn, FILE *file, long long int timeout, int flags) {
	C_UTILS_ARG_CHECK(conn->logger, 0, conn, file);

	// Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) {
		struct stat file_stats;	

		int file_fd = fileno(file);
		if (file_fd == -1) {
			C_UTILS_LOG_WARNING(conn->logger, "fileno: '%s'", strerror(errno));
			return 0;
		}

		if (fstat(file_fd, &file_stats) == -1) {
			C_UTILS_LOG_WARNING(conn->logger, "fstat: '%s'", strerror(errno));
			return 0;
		}

		size_t file_size = file_stats.st_size;
		char buf[send_buf_size];
		size_t buf_read = 0, total_sent = 0;

		C_UTILS_TEMP_FAILURE_RETRY(buf_read, fread(buf, 1, send_buf_size, file));
		while (buf_read > 0) {
			if (send_all(conn->sockfd, buf, buf_read, timeout, flags, conn->logger) != buf_read) {
				C_UTILS_LOG_WARNING(conn->logger, "c_utils_connection_send_file->send_all: 'Was unable to send all of message to %s'", conn->ip_addr);
				COND_RWLOCK_UNLOCK(conn->lock, conn->logger);
				return total_sent;
			}
			total_sent += buf_read;

			C_UTILS_TEMP_FAILURE_RETRY(buf_read, fread(buf, 1, send_buf_size, file));
		}
		
		if (total_sent != file_size) {
			C_UTILS_LOG_WARNING(conn->logger, "Was unable to send all of file to %s!File Size is %zu, but could only send %zu!",
				conn->ip_addr, file_size, total_sent);
		}

		return total_sent;
	} // Release Reader Lock
}

// Implement
size_t c_utils_connection_receive_file(struct c_utils_connection *conn, FILE *file, long long int timeout, int flags) {
	C_UTILS_ARG_CHECK(conn->logger, 0, conn, file);

	//Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) {
		struct stat file_stats;	

		int file_fd = fileno(file);
		if (file_fd == -1) {
			C_UTILS_LOG_WARNING(conn->logger, "fileno: '%s'", strerror(errno));
			return 0;
		}

		if (fstat(file_fd, &file_stats) == -1) {
			C_UTILS_LOG_WARNING(conn->logger, "fstat: '%s'", strerror(errno));
			return 0;
		}

		size_t buf_size = file_stats.st_blksize;
		char buf[buf_size];
		
		size_t total_received = 0, received;
		while ((received = timed_receive(conn->sockfd, buf, buf_size, timeout, flags, conn->logger)) > 0) {
			size_t written;
			C_UTILS_TEMP_FAILURE_RETRY(written, fwrite(buf, 1, received, file));
			if (written != received) {
				C_UTILS_LOG_ERROR(conn->logger, "fwrite: 'Written only %zu bytes, expected %zu bytes!\n%s'", written, received, strerror(errno));
				return total_received += written;
			}
			
			total_received += received;
		}

		return total_received
	} // Release Reader Lock
}

int c_utils_connection_select(struct c_utils_connection ***receivers, size_t *r_size, struct c_utils_connection ***senders, size_t *s_size,
 	long long int timeout, struct c_utils_logger *logger) {
	struct c_utils_connection **send_connections = NULL;
	struct c_utils_connection **recv_connections = NULL;

	/* 
		Initialized at top for goto statement consistency. The checks below ensure that at least one of the two groups are valid,
		and if not, logs it appropriately. This kind of check isn't generic enough to use C_UTILS_ARG_CHECK for, as it evaluates them individually,
		as well as collectively.

		Basically, we must ensure that at least one set of arguments are valid; either senders or receivers. If neither happens to be
		accurate, it is still possible that, say, 2/3 arguments are accurate and normally this would be very hard to debug. So, the solution
		here is to basically check each and log whether or not they are valid based on the checks.
	*/
	if ((!receivers || !r_size || !*r_size) && (!senders || !s_size || *s_size)) {
		C_UTILS_LOG_ERROR(logger, "Invalid Arguments: 'Receivers: %s;Receiver Size_ptr: %s;Receiver Size > 0: %s;"
				"Senders: %s;Sender Size_ptr: %s;Sender Size > 0: %s'\nMessage: '%s'", receivers ? "OK!" : "NULL",
						r_size ? "OK!" : "NO!", *r_size ? "OK!" : "NO!", senders ? "OK!" : "NULL", s_size ? "OK!" : "NULL",
								*s_size ? "OK!" : "NO!", "Neither receivers nor senders were valid!");
		goto error;
	}

	fd_set receive_set;
	fd_set send_set;
	struct c_utils_connection **r_conns = receivers ? *receivers : NULL;
	struct c_utils_connection **s_conns = senders ? *senders : NULL;
	size_t recv_size = r_size ? *r_size : 0;
	size_t send_size = s_size ? *s_size : 0;
	struct timeval tv = { .tv_sec = timeout };
	
	FD_ZERO(&receive_set);
	FD_ZERO(&send_set);
	
	/*
		For both sets, if either (or both) are not null, we must extract the file descriptors from
		each connection, and make the appropriate changes as well.
	*/
	int max_fd = 0, can_receive = 0, can_send = 0;
	int r_max_fd = add_valid_connections_to_fd_set(r_conns, recv_size, &receive_set);
	can_receive = (r_max_fd != -1);
	if (r_max_fd > max_fd)  
		max_fd = r_max_fd;
	
	
	int s_max_fd = add_valid_connections_to_fd_set(s_conns, send_size, &send_set);
	can_send = (s_max_fd != -1);
	if (s_max_fd > max_fd)  
		max_fd = s_max_fd;
	
	
	if (!can_receive && !can_send) {
		C_UTILS_LOG_WARNING(logger, "Was unable to find a valid receiver or sender connection!");
		goto error;
	}
	
	/*
		Now we block on the ready file descriptors until the passed timeout, or indefinitely until at least one
		can receive, just like a normal select syscall.
	*/
	size_t are_ready;
	C_UTILS_TEMP_FAILURE_RETRY(are_ready, select(max_fd + 1, &receive_set, &send_set, NULL, timeout < 0 ? NULL : &tv));
	if (are_ready <= 0) {
		if (!are_ready) C_UTILS_LOG_INFO(logger, "select: 'Timed out!'");
		else C_UTILS_LOG_WARNING(logger, "select: '%s'", strerror(errno));
		goto error;
	}
	
	recv_connections = malloc(sizeof(struct c_utils_connection *) * are_ready);
	if (!recv_connections) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	send_connections = malloc(sizeof(struct c_utils_connection *) * are_ready);
	if (!send_connections) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	/*
		In the below, we loop through each connection in both receiver and sender array, to check
		if they are available in their respective file descriptor set. The amount ready are also kept
		track of and are used to shrink to the size of the new array for each receiver and sender.

		Hence, if out of a group of 10 valid senders and receivers, only 3 senders and 7 receivers are ready,
		the initial size will be of size 10, but it will then be shrinked to their respective sides.
	*/
	size_t i = 0;
	for (; i < recv_size && r_conns; i++) {
		struct c_utils_connection *conn = r_conns[i];
		if (FD_ISSET(conn->sockfd, &receive_set))  
			recv_connections[can_receive++] = conn;
		
	}
	
	struct c_utils_connection **tmp_recv_connections = realloc(recv_connections, sizeof(struct c_utils_connection *) * can_receive);
	if (can_receive && !tmp_recv_connections) {
		C_UTILS_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		goto error;
	}
	recv_connections = tmp_recv_connections;

	for (i = 0; i < send_size && s_conns; i++) {
		struct c_utils_connection *conn = s_conns[i];
		if (FD_ISSET(conn->sockfd, &send_set))  
			send_connections[can_send++] = conn;
		
	}
	
	struct c_utils_connection **tmp_send_connections = realloc(send_connections, sizeof(struct c_utils_connection *) * can_send);
	if (can_send && !tmp_send_connections) {
		C_UTILS_LOG_ASSERT(logger, "realloc: '%s'", strerror(errno));
		goto error;
	}
	send_connections = tmp_send_connections;

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

struct c_utils_connection *c_utils_connection_reuse(struct c_utils_connection **connections, size_t size, int sockfd,
	unsigned int port, const char *ip_addr, struct c_utils_logger *logger) {
	C_UTILS_ARG_CHECK(logger, NULL, connections, size > 0, sockfd > -1, port > 0, ip_addr);
	
	size_t i = 0;
	for (;i < size; i++) {
		struct c_utils_connection *conn = connections[i];

		// Acquire Writer Lock
		SCOPED_LOCK0(conn->lock) { 
			if (conn && !conn->in_use) {
				conn->in_use = true;
				conn->sockfd = sockfd;
				conn->port = port;
				conn->logger = logger;
				strncpy(conn->ip_addr, ip_addr, INET_ADDRSTRLEN);
				return conn;
			}
		} // Release Writer Lock
	}

	return NULL;
}

bool c_utils_connection_set_sockfd(struct c_utils_connection *conn, int sockfd) {
	C_UTILS_ARG_CHECK(conn->logger, false, conn);

	// Acquire Writer Lock
	SCOPED_LOCK0(conn->logger) return conn->sockfd = sockfd, true;
}

int c_utils_connection_get_sockfd(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, -1, conn);

	// Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) return conn->sockfd;
}

bool c_utils_connection_set_ip_addr(struct c_utils_connection *conn, const char *ip_addr) {
	C_UTILS_ARG_CHECK(conn->logger, false, ip_addr);

	// Acquire Writer Lock
	SCOPED_LOCK0(conn->lock) return strncpy(conn->ip_addr, ip_addr, INET_ADDRSTRLEN), true;
}

const char *c_utils_connection_get_ip_addr(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, NULL, conn);

	// Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) return conn->ip_addr;
}

unsigned int c_utils_connection_get_port(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, 0, conn);

	// Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) return conn->port;
}

bool c_utils_connection_set_port(struct c_utils_connection *conn, unsigned int port) {
	C_UTILS_ARG_CHECK(conn->logger, false, conn, port > 0);

	// Acquire Writer Lock
	SCOPED_LOCK0(conn->lock) return conn->port = port, true;
}

struct c_utils_logger *c_utils_connection_get_logger(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, false, conn);

	// Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) return conn->logger;
}

bool c_utils_connection_set_logger(struct c_utils_connection *conn, struct c_utils_logger *logger) {
	C_UTILS_ARG_CHECK(logger, false, conn);

	// Acquire Writer Lock
	SCOPED_LOCK0(conn->lock) return conn->logger = logger, true;
}

bool c_utils_connection_init(struct c_utils_connection *conn, int sockfd, unsigned int port, const char *ip_addr, struct c_utils_logger *logger) {
	C_UTILS_ARG_CHECK(logger, false, conn, sockfd > -1, port > 0, ip_addr);

	// Acquire Writer Lock
	SCOPED_LOCK0(conn->lock) {
		if (conn->in_use) {
			C_UTILS_LOG_INFO(logger, "c_utils_connection_init: 'Connection already in use!'");
			return false;
		}

		conn->sockfd = sockfd;
		conn->port = port;
		strcpy(conn->ip_addr, ip_addr);
		if (logger)  
			conn->logger = logger;
		
		conn->in_use = true;
	} // Release Writer Lock

	return true;
}

bool c_utils_connection_in_use(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, false, conn);

	// Acquire Reader Lock
	SCOPED_LOCK1(conn->lock) return conn->in_use;
}

bool c_utils_connection_disconnect(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, false, conn);

	// Acquire Writer Lock
	SCOPED_LOCK0(conn->lock) {
		if (!conn->in_use)  
			return false;
		

		int has_closed;
		C_UTILS_TEMP_FAILURE_RETRY(has_closed, close(conn->sockfd));
		if (has_closed == -1) C_UTILS_LOG_WARNING(conn->logger, "close: '%s'", strerror(errno));

		conn->in_use = false;
	} // Release Writer Lock

	return true;
}

bool c_utils_connection_destroy(struct c_utils_connection *conn) {
	C_UTILS_ARG_CHECK(conn->logger, false, conn);

	c_utils_connection_disconnect(conn);

	c_utils_scoped_lock_destroy(conn->lock);
	free(conn);

	return true;
}
