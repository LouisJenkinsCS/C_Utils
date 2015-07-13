#include <MU_Logger.h>
#include <pthread.h>
#include <NU_Server.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;

static NU_Server_t *server = NULL;

static const int username_max_length = 16;
static const unsigned int avg_timeout = 30;
static const unsigned int generous_timeout = 60;
static const unsigned int port_num = 10000;
static const unsigned int max_clients = 2;

typedef struct {
	NU_Connection_t *client;
	char username[username_max_length];
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
	char *prefix = " says: \n\"";
	char *suffix = "\"\n";
	size_t username_prefix_size = strlen(task->sender->username) + strlen(prefix);
	char *username_prefix = calloc(1, username_prefix_size + 1);
	strcpy(username_prefix, task->sender->username);
	strcat(username_prefix, prefix);
	username_prefix[username_prefix_size] = '\0';
	char message[task->buffer_size + 1];
	char redirect_message[task->buffer_size + username_prefix_size + 1];
	while(1){
		size_t received = NU_Server_receive(server, task->sender->client, msg, task->buffer_size, task->timeout));
		if(!received){
			break;
		}
		msg[received] = '\0';
		strtok(msg, "\r\n");
		strcpy(redirect_message, username_prefix);
		strcat(redirect_message, msg);
		strcat(redirect_message, suffix);
		//MU_LOG_VERBOSE(logger, "%s\n", new_msg);
		if(!NU_Server_send(server, task->receiver->client, (const char *)redirect_message, strlen(redirect_message), task->timeout)) {
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
	char username[username_max_length];
	size_t sent = NU_Server_send(server, client, "Username:", strlen("Username:") avg_timeout);
	MU_ASSERT(sent, logger, "Client timed out or disconnected while asking for username!\n");
	size_t received = NU_Server_receive(server, client, username, username_max_length, avg_timeout);
	MU_ASSERT(received, logger, "Client timed out or disconnected waiting to obtain username!\n");
	strtok(username, "\r\n");
	return wrap_client(client, username);
}

int main(void){
	logger = calloc(1, sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "NU_Server_Test.log", "w", MU_ALL);
	server = NU_Server_create(max_clients, 1);
	NU_Bound_Socket_t *bsock = NU_Server_bind(server, "10.0.2.15", port_num, max_clients, NU_NONE);
	MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
	pthread_t thread_one, thread_two;
	Client_Wrapper_t **wrappers = malloc(sizeof(Client_Wrapper_t *) * max_clients);
	int i = 0;
	for(;i < max_clients; i++){
		MU_DEBUG("Accepting a new client...\n");
		Client_Wrapper_t *wrapper = obtain_client(bsock);
		MU_ASSERT(wrapper, logger, "Failed to obtain wrapper for client socket!\n");
		wrappers[i] = wrapper;
		MU_DEBUG("Connected to %s!\n", wrapper->username);
	}
	Thread_Task_t *wrapper_one_task = new_task(wrappers[0], wrappers[1], 4 * NU_KILOBYTE, avg_timeout);
	Thread_Task_t *wrapper_two_task = new_task(wrappers[1], wrappers[0], 4 * NU_KILOBYTE, avg_timeout);
	int created_thread = pthread_create(&thread_one, NULL, redirector_thread, wrapper_one_task);
	MU_ASSERT(created_thread, logger, "pthread_create: \"%s\"\n", strerror(created_thread));
	created_thread = pthread_create(&thread_two, NULL, redirector_thread, wrapper_two_task);
	MU_ASSERT(created_thread, logger, "pthread_create: \"%s\"\n", strerror(created_thread));
	MU_DEBUG("Threads started...\n");
	int joined_thread = pthread_join(thread_one, NULL);
	MU_ASSERT(joined_thread, logger, "pthread_join: \"%s\"\n", strerror(joined_thread));
	joined_thread = pthread_join(thread_two, NULL);
	MU_ASSERT(joined_thread, logger, "pthread_join: \"%s\"\n", strerror(joined_thread));
	char *end_result = NU_Server_about(server);
	MU_DEBUG("%s\n", end_result);
	free(end_result);
	NU_Server_log(server, "%s\n", "Server being shutdown!");
	NU_Server_destroy(server, NULL);
}