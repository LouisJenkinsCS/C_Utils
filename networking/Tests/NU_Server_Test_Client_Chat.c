#include <MU_Logger.h>
#include <pthread.h>
#include <NU_Server.h>
#include <unistd.h>

static struct c_utils_logger *logger = NULL;

static NU_Server_t *server = NULL;

static const int username_max_length = 16;
static const unsigned int avg_timeout = 30;
static const unsigned int generous_timeout = 60;
static const unsigned int port_num = 10000;
static const unsigned int max_clients = 2;
static const unsigned int max_bsocks = 1;

typedef struct {
	NU_Connection_t *client;
	char username[16];
} Client_Wrapper_t;

static Client_Wrapper_t *wrap_client(NU_Connection_t *client, char *username){
	Client_Wrapper_t *wrapper = calloc(1, sizeof(Client_Wrapper_t));
	wrapper->client = client;
	strcpy(wrapper->username, username);
	return wrapper;
}

typedef struct {
	size_t buffer_size;
	unsigned int timeout;
	Client_Wrapper_t *receiver;
	Client_Wrapper_t *sender;
} Thread_Task_t;

static Thread_Task_t *new_task(Client_Wrapper_t *receiver, Client_Wrapper_t *sender, size_t buffer_size, unsigned int timeout){
	Thread_Task_t *task = calloc(1, sizeof(Thread_Task_t));
	task->receiver = receiver;
	task->sender = sender;
	task->buffer_size = buffer_size;
	task->timeout = timeout;
	return task;
}

static void *redirector_thread(void *args){
	Thread_Task_t *task = args;
	char *prefix = " says: \n";
	char *suffix = "\n";
	size_t username_prefix_size = strlen(task->sender->username) + strlen(prefix);
	char *username_prefix = calloc(1, username_prefix_size + 1);
	strcpy(username_prefix, task->sender->username);
	strcat(username_prefix, prefix);
	username_prefix[username_prefix_size] = '\0';
	char msg[task->buffer_size + 1];
	char redirect_message[task->buffer_size + username_prefix_size + 1];
	while(1){
		size_t received = NU_Connection_receive(task->sender->client, msg, task->buffer_size, task->timeout, 0);
		if(!received){
			break;
		}
		msg[received] = '\0';
		strtok(msg, "\r\n");
		strcpy(redirect_message, username_prefix);
		strcat(redirect_message, msg);
		strcat(redirect_message, suffix);
		//C_UTILS_LOG_VERBOSE(logger, "%s\n", new_msg);
		if(!NU_Connection_send(task->receiver->client, redirect_message, strlen(redirect_message), task->timeout, 0)){
			break;
		}
	}
	NU_Server_disconnect(server, task->sender->client);
	free(task);
	return NULL;
}

static Client_Wrapper_t *obtain_client(NU_Bound_Socket_t *bsock){
	NU_Connection_t *client = NU_Server_accept(server, bsock, generous_timeout);
	MU_ASSERT(client, logger, "Client did not connect in time!\n");
	NU_Connection_send(client, "\255\253\34", 3, avg_timeout, 0);
	char username[username_max_length];
	size_t sent = NU_Connection_send(client, "Username:", strlen("Username:"), avg_timeout, 0);
	MU_ASSERT(sent, logger, "Client timed out or disconnected while asking for username!\n");
	size_t received = NU_Connection_receive(client, username, username_max_length, avg_timeout, 0);
	MU_ASSERT(received, logger, "Client timed out or disconnected waiting to obtain username!\n");
	strtok(username, "\r\n");
	return wrap_client(client, username);
}

int main(void){
	logger = MU_Logger_create("./Net_Utils/Logs/NU_Server_Telnet_Client_Chat.log", "w", MU_ALL);
	server = NU_Server_create(max_clients, max_bsocks, true);
	NU_Bound_Socket_t *bsock = NU_Server_bind(server, max_clients, port_num, "192.168.1.112");
	MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
	pthread_t thread_one, thread_two;
	Client_Wrapper_t **wrappers = malloc(sizeof(Client_Wrapper_t *) * max_clients);
	int i = 0;
	for(;i < max_clients; i++){
		MU_DEBUG("Accepting a new client...");
		Client_Wrapper_t *wrapper = obtain_client(bsock);
		MU_ASSERT(wrapper, logger, "Failed to obtain wrapper for client socket!\n");
		wrappers[i] = wrapper;
		MU_DEBUG("Connected to %s!", wrapper->username);
	}
	Thread_Task_t *wrapper_one_task = new_task(wrappers[0], wrappers[1], 4 * 1024, avg_timeout);
	Thread_Task_t *wrapper_two_task = new_task(wrappers[1], wrappers[0], 4 * 1024, avg_timeout);
	int failed_create = pthread_create(&thread_one, NULL, redirector_thread, wrapper_one_task);
	MU_ASSERT(!failed_create, logger, "pthread_create: %s", strerror(failed_create));
	failed_create = pthread_create(&thread_two, NULL, redirector_thread, wrapper_two_task);
	MU_ASSERT(!failed_create, logger, "pthread_create: %s", strerror(failed_create));
	MU_DEBUG("Threads started...\n");
	int join_failed = pthread_join(thread_one, NULL);
	MU_ASSERT(!join_failed, logger, "pthread_join: %s", strerror(join_failed));
	join_failed = pthread_join(thread_two, NULL);
	MU_ASSERT(!join_failed, logger, "pthread_join: %s", strerror(join_failed));
	NU_Server_destroy(server);
	return EXIT_SUCCESS;
}