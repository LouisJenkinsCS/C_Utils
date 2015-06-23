#include <Misc_Utils.h>
#include <NU_Server.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;

static char *child_or_parent(int pid){
	return pid ? "Parent" : "Child";
}

int main(void){
	logger = calloc(1, sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "NU_Server_Test_LOg.txt", "w", MU_ALL);
	NU_Server_t *server = NU_Server_create(0);
	NU_Bound_Socket_t *bsock = NU_Server_bind(server, 10000, 1, 0);
	MU_DEBUG("Accepting client one...\n");
	NU_Client_Socket_t *client_one = NU_Server_accept(server, bsock, 60);
	MU_DEBUG("Connected to client one!\n");
	MU_DEBUG("Accepting client two...\n");
	NU_Client_Socket_t *client_two = NU_Server_accept(server, bsock, 60);
	MU_DEBUG("Connected to client two!\n");
	MU_DEBUG("Forking...\n");
	int pid = fork();
	if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if(!pid){
		const char *id = child_or_parent(pid);
		MU_DEBUG("Inside of %s;See logs for results!\n", id);
		const int timeout = 10, buffer_size = 1024;
		const char *message = "Hello World!\n";
		int message_length = strlen(message);
		MU_LOG_INFO(logger, "%s: started!\n", id);
		MU_LOG_INFO(logger, "%s: receiving a message from client_one with %d timeout!\n", id, timeout);
		const char *retmsg = NU_Server_receive(server, client_one, buffer_size, timeout);
		MU_LOG_VERBOSE(logger, "client_one message: \"%s\"\n", retmsg);
		MU_LOG_INFO(logger, "%s: sending the message \"%s\" to client_one\n", id, message);
		int retval = NU_Server_send(server, client_one, message, timeout);
		MU_ASSERT(retval == message_length, logger, "bytes_read: %d, expected %d\n", retval, message_length);
	}
	else{
		const char *id = child_or_parent(pid);
		MU_DEBUG("Inside of %s;See logs for results!\n", id);
		const int timeout = 10, buffer_size = 1024;
		const char *message = "Goodbyte World!\n";
		int message_length = strlen(message);
		MU_LOG_INFO(logger, "%s: started!\n", id);
		MU_LOG_INFO(logger, "%s: receiving a message from client_two with %d timeout!\n", id, timeout);
		const char *retmsg = NU_Server_receive(server, client_two, buffer_size, timeout);
		MU_LOG_VERBOSE(logger, "client_one message: \"%s\"\n", retmsg);
		MU_LOG_INFO(logger, "%s: sending the message \"%s\" to client_two\n", id, message);
		int retval = NU_Server_send(server, client_two, message, timeout);
		MU_ASSERT(retval == message_length, logger, "bytes_read: %d, expected %d\n", retval, message_length);
	}
}