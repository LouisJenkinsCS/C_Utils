#include <Misc_Utils.h>
#include <NU_Server.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;

static char *child_or_parent(int pid){
	return pid ? "Parent" : "Child";
}

int main(void){
	const int buffer_size = 1024;
	const size_t timeout = 30;
	logger = calloc(1, sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "NU_Server_Test_LOg.txt", "w", MU_ALL);
	NU_Server_t *server = NU_Server_create(0);
	NU_Bound_Socket_t *bsock = NU_Server_bind(server, 10000, 1, 0);
	MU_DEBUG("Accepting client one...\n");
	NU_Client_Socket_t *client_one = NU_Server_accept(server, bsock, 60);
	if(!client_one){
		MU_DEBUG("Client one did not connect in time!\n");
		return 0;
	}
	MU_DEBUG("Connected to client one!\n");
	MU_DEBUG("Accepting client two...\n");
	NU_Client_Socket_t *client_two = NU_Server_accept(server, bsock, 60);
	if(!client_two){
		MU_DEBUG("Client two did not connect in time!\n");
		return 0;
	}
	MU_DEBUG("Connected to client two!\n");
	MU_DEBUG("Forking...\n");
	int pid = fork();
	if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if(!pid){
		int retval;
		while(1){
			char *user_name_prefix = "client_one: ";
			const char *msg = NU_Server_receive(server, client_two, buffer_size, timeout);
			if(!msg) break;
			char *new_msg = calloc(1, strlen(msg) + strlen(user_name_prefix) + 1);
			strcat(new_msg, user_name_prefix);
			strcat(new_msg, msg);
			if(!(retval = NU_Server_send(server, client_one, new_msg, timeout))){
				break;
			}
		}
	}
	else{
		int retval;
		while(1){
			char *user_name_prefix = "client_two: ";
			const char *msg = NU_Server_receive(server, client_one, buffer_size, timeout);
			if(!msg) break; 
			char *new_msg = calloc(1, strlen(msg) + strlen(user_name_prefix) + 1);
			strcat(new_msg, user_name_prefix);
			strcat(new_msg, msg);
			if(!(retval = NU_Server_send(server, client_two, new_msg, timeout))){
				break;
			}
		}
	}
	MU_DEBUG("%s\n", NU_Server_about(server));
}