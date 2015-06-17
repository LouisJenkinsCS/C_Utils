#include <Net_Utils.h>
#include <Misc_Utils.h>

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

static size_t send_all(int sockfd, char *message, unsigned int timeout){
	size_t buffer_size = strlen(message), total_sent = 0, data_left = buffer_size;
	int retval, recently_sent = 0;
	struct timeval tv;
	fd_set can_send;
	tv.tv_sec = timeout;
	tv_tv_usec = 0;
	FD_ZERO(&can_send);
	FD_SET(sockfd, &can_send)
	while(total_sent <= buffer_size){
		if((retval = select(sockfd+1, NULL, &can_send, NULL, &tv)) < 0){
			MU_LOG_WARNING(logger, "Select returned the error: \"%s\"", gai_strerror(retval));
			break;
		}
		if((recently_sent = send(sockfd, message[total_sent], data_left, 0)) <= 0){
			if(!recently_receieved) MU_LOG_INFO(logger, "The receiver disconnected from the stream!\n");
			else MU_LOG_WARNING(logger, "send_all: \"%s\"\n", gai_strerror(recently_receieved));
			break;
		}
		total_sent += recently_sent;
		data_left -= recently_sent;
	}
	FD_CLR(sockfd, &can_send);
	return total_sent;
}

static size_t receive_all(int sockfd, size_t buffer_size, unsigned int timeout){
	size_t total_received = 0, data_left = buffer_size;
	int retval, recently_receieved = 0;
	char *message = malloc(buffer_size);
	struct timeval tv;
	fd_set can_receive;
	tv.tv_sec = timeout;
	tv_tv_usec = 0;
	FD_ZERO(&can_receive);
	FD_SET(sockfd, &can_receive);
	while(buffer_size >= total_received){
		if((retval = select(sockfd + 1, &can_receive, NULL, NULL, &tv)) < 0){
			MU_LOG_WARNING(logger, "Select returned the error: \"%s\"", gai_strerror(retval));
			break;
		}
		if((recently_receieved = recv(sockfd, message[total_received], data_left, 0)) <= 0){
			if(!recently_receieved) MU_LOG_INFO(logger, "The sender disconnected from the stream!\n");
			else MU_LOG_WARNING(logger, "receive_all: \"%s\"\n", gai_strerror(recently_receieved));
			break;
		}
		total_received += recently_receieved;
		data_left -= recently_sent;
	}
	FD_CLR(sockfd, &can_send);
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

NU_Client_t *NU_Client_create(char *host, char *port, int flags){
	MU_Client_t *client = malloc(sizeof(MU_Client_t));
	client->messages_sent = client->messages_received = client->data_sent = client->data_received = 0;
	struct addrinfo socket_options, *results;
	int retval;
	memset(&socket_options, 0, sizeof(socket_options));
	socket_options.ai_family = AF_INET;
	socket_options.ai_socktype = SOCK_STREAM;
	if((retval = getaddrinfo(host, port, &socket_options, &results)) != 0){
		MU_LOG_WARNING(logger, "Unable to get addrinfo: %s\n", gai_strerror(retval));
		free(client);
		return NULL;
	}
	if((client->sockfd = get_client_socket(results)) == -1){
		MU_LOG_WARNING(logger, "Was unable to find a valid address!\n");
		freeaddrinfo(results);
		free(client);
		return NULL;
	}
	freeaddrinfo(results);
	return client;
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
	return 1;
}

char *NU_Client_recieve(NU_Client_t *client, size_t buffer_size, unsigned int timeout){
	size_t result = receive_all(client->sockfd, buffer_size, timeout);
	if(result != buffer_size){
		MU_LOG_WARNING(logger, "Was unable to recieve enough data to fill the buffer! Total received: %d, Buffer Size: %d\n", result, buffer_size);
		return 0;
	}
	return 1;
}

char *MU_Client_about(MU_Client_t *client){
	
}
