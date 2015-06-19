#include <Net_Utils.h>
#include <Misc_Utils.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for Net_Util's logger!!!");
		return;
	}
	MU_Logger_Init(logger, "Net_Utils_Log.txt", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger, 1);
	logger = NULL;
}

/* Server-Client Helper functions defined below! */

static int reset_client(MU_Client_t *client){
	const int hostname_length = 100, port_length = 5;
	memset(client->hostname, '\0', hostname_length);
	memset(client->port, '\0', port_length);
	client->data->messages_sent = client->data->messages_received = client->data->bytes_sent = client->data->bytes_received = client->is_connected = 0;
	return 1;
}

static size_t send_all(int sockfd, char *message, unsigned int timeout){
	size_t buffer_size = strlen(message), total_sent = 0, data_left = buffer_size;
	int retval, recently_sent = 0;
	struct timeval tv;
	fd_set can_send, can_send_copy;
	tv.tv_sec = timeout;
	tv_tv_usec = 0;
	FD_ZERO(&can_send);
	FD_SET(sockfd, &can_send)
	while(buffer_size > total_sent){
		can_send_copy = can_send;
		// Restart timeout.
		tv.tv_sec = timeout;
		if((retval = TEMP_FAILURE_RETRY(select(sockfd+1, NULL, &can_send_copy, NULL, &tv))) <= 0){
			if(!retval){
				MU_LOG_INFO(logger, "select: timed out!\n");
				break;
			}
			MU_LOG_ERROR(logger, "select: \"%s\"", gai_strerror(retval));
			break;
		}
		if((recently_sent = send(sockfd, message[total_sent], data_left, 0)) <= 0){
			if(!recently_receieved) MU_LOG_INFO(logger, "send: disconnected from the stream!\n");
			else MU_LOG_ERROR(logger, "send: \"%s\"\n", gai_strerror(recently_receieved));
			break;
		}
		total_sent += recently_sent;
		data_left -= recently_sent;
	}
	return total_sent;
}

static size_t receive_all(int sockfd, size_t buffer_size, unsigned int timeout){
	size_t total_received = 0, data_left = buffer_size;
	int retval, recently_receieved = 0;
	char *message = malloc(buffer_size);
	struct timeval tv;
	fd_set can_receive, can_receive_copy;
	tv.tv_sec = timeout;
	tv_tv_usec = 0;
	FD_ZERO(&can_receive);
	FD_SET(sockfd, &can_receive);
	while(buffer_size > total_received){
		can_receive_copy = can_receive;
		// After every iteration, timeout is reset.
		tv.tv_sec = timeout;
		if((retval = TEMP_FAILURE_RETRY(select(sockfd + 1, &can_receive_copy, NULL, NULL, &tv))) <= 0){
			if(!retval){
				MU_LOG_INFO(logger, "select: timed out!\n");
				break;
			}
			MU_LOG_ERROR(logger, "select: \"%s\"", gai_strerror(retval));
			break;
		}
		if((recently_receieved = recv(sockfd, message[total_received], data_left, 0)) <= 0){
			if(!recently_receieved) MU_LOG_INFO(logger, "recv: disconnected from the stream!\n");
			else MU_LOG_ERROR(logger, "recv: \"%s\"\n", gai_strerror(recently_receieved));
			break;
		}
		total_received += recently_receieved;
		data_left -= recently_sent;
	}
	return total_received;
}

static int get_client_socket(struct addrinfo **results){
	struct addrinfo *current = NULL;
	int sockfd = 0, iteration = 0, retval;
	for(current = results; current; current = current->ai_next){
		if((sockfd = socket(current->ai_family, current->ai_socktype, current->ai_protocol)) < 0) {
			MU_LOG_VERBOSE(logger, "Skipped result with error \"%s\": Iteration #%d\n", gai_strerror(sockfd), ++iteration);
			continue;
		}
		MU_LOG_VERBOSE(logger, "Obtained a socket from a result: Iteration #%d\n", ++i);
		if((retval = connect(sockfd, current->ai_addr, current->ai_addrlen)) < 0){
			close(sockfd);
			MU_LOG_VERBOSE(logger, "Unable to connect to socket with error \"%s\": Iteration #%d\n", gai_strerror(retval), ++iteration);
			continue;
		}
		break;
	}
	if(!current) return -1;
	return sockfd;
}

/* Client functions defined below! */

NU_Client_t *NU_Client_create(int flags){
	MU_Client_t *client = calloc(1, sizeof(MU_Client_t));
	if(!client) return NULL;
	client->timestamp = Misc_Utils_get_timestamp();
	return client;
}

int MU_Client_connect(MU_Client_t *client, char *host, char *port, int flags){
	client->hostname = host;
	client->port = port;
	struct addrinfo socket_options, *results;
	int retval;
	memset(&socket_options, 0, sizeof(socket_options));
	socket_options.ai_family = AF_INET;
	socket_options.ai_socktype = SOCK_STREAM;
	if((retval = getaddrinfo(host, port, &socket_options, &results)) != 0){
		MU_LOG_WARNING(logger, "Unable to get addrinfo: %s\n", gai_strerror(retval));
		return NULL;
	}
	if((client->sockfd = get_client_socket(results)) == -1){
		MU_LOG_WARNING(logger, "Was unable to find a valid address!\n");
		freeaddrinfo(results);
		return NULL;
	}
	freeaddrinfo(results);
	client->
}

int NU_Client_send(NU_Client_t *client, char *message, unsigned int timeout){
	size_t buffer_size = strlen(message);
	size_t result = send_all(client->sockfd, message, &data_sent, timeout);
	client->messages_sent += 1;
	client->data_sent += result;
	if(result != buffer_size){
		MU_LOG_WARNING(logger, "Was unable to send all data to host!Total Sent: %d, Message Size: %d\n", result, buffer_size);
		return 0;
	}
	client->data->bytes_sent += result;
	client->data->messages_sent++;
	return 1;
}

char *NU_Client_recieve(NU_Client_t *client, size_t buffer_size, unsigned int timeout){
	size_t result = receive_all(client->sockfd, buffer_size, timeout);
	if(result != buffer_size){
		MU_LOG_WARNING(logger, "Was unable to recieve enough data to fill the buffer! Total received: %d, Buffer Size: %d\n", result, buffer_size);
		return 0;
	}
	client->data->bytes_received += result;
	client->data->messages_received++;
	return 1;
}

char *MU_Client_about(MU_Client_t *client){
	char *message, *timestamp = Misc_Utils_get_timestamp();
	asprintf(&message, "Connected to: %s:%s\nMessages sent: %d; Total bytes: %d\nMessages received: %d; Total bytes: %d\n
		Connected from time %s to %s\n", client->host, client->port, client->data->messages_sent, client->data->bytes_sent,
		client->data->messages_received, client->data->bytes_received, client->timestamp, timestamp);
	free(timestamp);
	return message;
}

int MU_Client_shutdown(MU_Client_t *client){
	shutdown(client->sockfd, 2);
	MU_LOG_INFO(logger, "Client shutdown!\n");
	reset_client(client);
	return 1;
}

int MU_Client_destroy(MU_Client_t *client){
	MU_Client_shutdown(client);
	free(client->timestamp);
	free(client);
	MU_LOG_INFO(logger, "Client destroyed!\n");
	return 1;
}
