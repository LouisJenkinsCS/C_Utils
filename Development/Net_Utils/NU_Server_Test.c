#include <Misc_Utils.h>
#include <pthread.h>
#include <NU_Server.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;

NU_Server_t *server = NULL;

const int username_max_length = 16;
const unsigned int avg_timeout = 30;
const unsigned int generous_timeout = 60;
const unsigned int port_num = 10000;
const unsigned int queue_max = 2;

typedef struct {
	NU_Client_Socket_t *client;
	char *username;
} Client_Wrapper_t;

static Client_Wrapper_t *wrap_client(NU_Client_Socket_t *client, char *username){
	Client_Wrapper_t *wrapper = calloc(1, sizeof(Client_Wrapper_t));
	wrapper->client = client;
	wrapper->username = username;
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
	while(1){
		const char *msg = NU_Server_receive(server, task->sender->client, task->buffer_size, task->timeout);
		if(!msg) break;
		strtok(msg, "\r\n");
		size_t new_msg_size = strlen(username_prefix) + strlen(msg) + strlen(suffix);
		char *new_msg = calloc(1, new_msg_size + 1);
		strcpy(new_msg, username_prefix);
		strcat(new_msg, msg);
		strcat(new_msg, suffix);
		new_msg[new_msg_size] = '\0';
		MU_LOG_VERBOSE(logger, "%s\n", new_msg);
		if(!NU_Server_send(server, task->receiver->client, (const char *)new_msg, task->timeout)) {
			free(new_msg);
			break;
		} free(new_msg);
	}
	char *end_msg;
	asprintf(&end_msg, "%s: Shutting down!\n", task->sender->username);
	NU_Server_disconnect(server, task->sender->client, (const char *)end_msg);
	free(task->sender->username);
	free(task);
	return NULL;
}

static Client_Wrapper_t *obtain_client(NU_Bound_Socket_t *bsock){
	NU_Client_Socket_t *client = NU_Server_accept(server, bsock, 60);
	MU_ASSERT(client, logger, "Client did not connect in time!\n");
	MU_ASSERT(NU_Server_send(server, client, "Username:", avg_timeout), logger,
		"Client timed out or disconnected while asking for username!\n");
	char *username = strdup(NU_Server_receive(server, client, username_max_length, avg_timeout));
	MU_ASSERT(username, logger, "Client timed out or disconnected waiting to obtain username!\n");
	strtok(username, "\r\n");
	return wrap_client(client, username);
}

int main(void){
	logger = calloc(1, sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "NU_Server_Test_LOg.txt", "w", MU_ALL);
	server = NU_Server_create(0);
	NU_Bound_Socket_t *bsock = NU_Server_bind(server, port_num, queue_max, NU_NONE);
	MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
	pthread_t thread_one, thread_two;
	Client_Wrapper_t **wrappers = malloc(sizeof(Client_Wrapper_t *) * queue_max);
	int i = 0;
	for(;i < queue_max; i++){
		MU_DEBUG("Accepting a new client...\n");
		Client_Wrapper_t *wrapper = obtain_client(bsock);
		MU_ASSERT(wrapper, logger, "Failed to obtain wrapper for client socket!\n");
		wrappers[i] = wrapper;
		MU_DEBUG("Connected to %s!\n", wrapper->username);
	}
	Thread_Task_t *wrapper_one_task = new_task(wrappers[0], wrappers[1], 1024 * 4, avg_timeout);
	Thread_Task_t *wrapper_two_task = new_task(wrappers[1], wrappers[0], 1024 * 4, avg_timeout);
	MU_ASSERT(!pthread_create(&thread_one, NULL, redirector_thread, wrapper_one_task), logger, "pthread_create: \"%s\"\n", strerror(errno));
	MU_ASSERT(!pthread_create(&thread_two, NULL, redirector_thread, wrapper_two_task), logger, "pthread_create: \"%s\"\n", strerror(errno));
	MU_DEBUG("Threads started...\n");
	pthread_join(thread_one, NULL);
	pthread_join(thread_two, NULL);
	char *end_result = NU_Server_about(server);
	MU_DEBUG("%s\n", end_result);
	free(end_result);
	NU_Server_log(server, "%s\n", "Server being shutdown!");
	NU_Server_destroy(server, NULL);
}