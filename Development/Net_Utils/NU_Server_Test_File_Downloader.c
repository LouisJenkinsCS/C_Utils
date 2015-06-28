#include <Misc_Utils.h>
#include <pthread.h>
#include <NU_Server.h>
#include <unistd.h>

MU_Logger_t *logger = NULL;
NU_Server_t *server = NULL;
const unsigned int timeout = 60;
const size_t buffer_size = 1024;

static const char *assert_get_info(NU_Client_Socket_t *client, const char *inquiry){
  size_t bytes_sent = NU_Server_send(server, client, (const char *)client_inquiry, timeout);
  MU_ASSERT(bytes_sent == strlen(client_inquiry), logger,
	    "Not all bytes were sent to client!\nbytes_sent: %zu, message_bytes: %zu\n", bytes_sent, strlen(client_inquiry));
  const char *retval = NU_Server_receive(server, client, buffer_size, timeout);
  MU_ASSERT(retval, logger, "Client either disconnected or did not send result in time!\n");
  return retval;
}

static void send_file(NU_Client_Socket_t *client){
  const char *filename = assert_get_info(client, "Filename...\n");
  FILE *file = fopen(filename, "r");
  MU_ASSERT(file, logger, "Invalid filename and/or file_mode!\nfilename: \"%s\";file_mode: \"READ\"\n", filename);
  NU_Server_send_file(server, client, file, buffer_size, timeout);
}

static void recv_file(NU_Client_Socket_t *client){
  const char *filename = assert_get_info(client, "Filename...\n");
  FILE *file = fopen(filename, "w");
  MU_ASSERT(file, logger, "Invalid filename and/or file_mode!\nfilename: \"%s\";file_mode: \"WRITE\"\n", filename);
  NU_Server_receive_to_file()
}

int main(void){
  size_t bytes_sent;
  char *client_inquiry;
  const char *filename;
  FILE *file;
  logger = calloc(1, sizeof(MU_Logger_t));
  MU_Logger_Init(logger, "NU_Server_Test_Log.txt", "w", MU_ALL);
  server = NU_Server_create(0);
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, "10.0.2.15", port_num, queue_max, NU_NONE);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  NU_Client_Socket_t *client_one = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client_one, logger, "Client_One did not connect in time!\n");
  const char *client_one_mode = assert_get_info(client_one, "Send or Recv?[S/R]\n");
  const char client_one_option = tolower(*client_one_mode);
  MU_ASSERT(client_one_mode == 'r' || client_one_mode == 's', logger, "Invalid option: %c\n", client_one_mode);
  if(client_one_mode == 'r')  filename = assert_get_info(client_one, ")
  NU_Client_Socket_t *client_two = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client_two, logger, "Client_Two did not connect in time!\n");
  const char client_two_option = mode == 'r' ? 's' || 'r';
  NU_Server_destroy(server, "Shutting down!\n");
}