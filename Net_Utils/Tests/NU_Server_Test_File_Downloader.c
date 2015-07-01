#include <MU_Logger.h>
#include <pthread.h>
#include <stdio.h>
#include <NU_Server.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;
static NU_Server_t *server = NULL;
static const unsigned int timeout = 60;
static const size_t buffer_size = 1024;
static const unsigned int port_num = 10000;
static const unsigned int queue_max = 2;

static const char *assert_get_info(NU_Client_Socket_t *client, const char *inquiry){
  size_t bytes_sent = NU_Server_send(server, client, (const char *)inquiry, timeout);
  MU_ASSERT((bytes_sent == strlen(inquiry)), logger,
	    "Not all bytes were sent to client!\nbytes_sent: %zu, message_bytes: %zu\n", bytes_sent, strlen(inquiry));
  const char *retval = NU_Server_receive(server, client, buffer_size, timeout);
  MU_ASSERT(retval, logger, "Client either disconnected or did not send result in time!\n");
  return retval;
}

static void redirect_file(NU_Client_Socket_t *client_one, NU_Client_Socket_t *client_two){
  NU_Client_Socket_t **arr = malloc(sizeof(NU_Client_Socket_t *) * 2);
  arr[0] = client_one;
  arr[1] = client_two;
  size_t size = 2;
  NU_Client_Socket_t **ready = NU_Server_select_receive(server, arr, &size, timeout);
  MU_ASSERT(ready && size, logger, "Neither clients were sending any data!\n");
  NU_Client_Socket_t *sender = arr[0] == client_one ? client_one : client_two;
  NU_Client_Socket_t *receiver = arr[0] == client_one ? client_two : client_one;
  const char *filename = assert_get_info(sender, "Filename...\n");
  FILE *file = tmpfile();
  MU_ASSERT(file, logger, "Was unable to create temporary file!\n");
  size_t retval = NU_Server_receive_to_file(server, sender, file, buffer_size, 1, timeout);
  MU_ASSERT(retval, logger, "Was unable to receive file from sender!\n");
  retval = NU_Server_send(server, receiver, filename, timeout);
  MU_ASSERT(retval, logger, "Was unable to send filename to receiver!\n");
  retval = NU_Server_send_file(server, receiver, file, timeout);
  MU_ASSERT(retval, logger, "Was unable to send file to receiver!\n");
}

int main(void){
  size_t bytes_sent;
  char *client_inquiry;
  logger = calloc(1, sizeof(MU_Logger_t));
  MU_Logger_Init(logger, "NU_Server_Test_File_Downloader_Log.txt", "w", MU_ALL);
  server = NU_Server_create(0);
  MU_ASSERT(server, logger, "Was unable to create server!\n");
  char ip_addr[INET_ADDRSTRLEN];
  MU_DEBUG("IP Address:");
  scanf("%s", ip_addr);
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, ip_addr, port_num, queue_max, NU_NONE);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  NU_Client_Socket_t *client_one = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client_one, logger, "Client_One did not connect in time!\n");
  MU_DEBUG("Client One connected...\n");
  NU_Client_Socket_t *client_two = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client_two, logger, "Client_Two did not connect in time!\n");
  MU_DEBUG("Client Two connected...\n");
  redirect_file(client_one, client_two);
  NU_Server_destroy(server, "Shutting down!\n");
}