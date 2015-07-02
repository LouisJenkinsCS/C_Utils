#include <MU_Logger.h>
#include <pthread.h>
#include <stdio.h>
#include <NU_Client.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;
static NU_Client_t *client = NULL;
static const unsigned int timeout = 60;
static const size_t buffer_size = 1024;
static const unsigned int port_num = 10000;

static const char *assert_get_info(NU_Server_Socket_t *server, const char *inquiry){
  size_t bytes_sent = NU_Client_send(client, server, (const char *)inquiry, timeout);
  MU_ASSERT((bytes_sent == strlen(inquiry)), logger,
	    "Not all bytes were sent to client!\nbytes_sent: %zu, message_bytes: %zu\n", bytes_sent, strlen(inquiry));
  const char *retval = NU_Client_receive(client, server, buffer_size, timeout);
  MU_ASSERT(retval, logger, "Client either disconnected or did not send result in time!\n");
  return retval;
}

static void send_file(NU_Server_Socket_t *server){
  char *filepath = calloc(1, 255);
  MU_DEBUG("File Path:");
  scanf("%s", filepath);
  FILE *file = fopen(filepath, "rb");
  MU_ASSERT(file, logger, "Was unable to open file: \"%s\" with mode \"%s\"\n", filepath, "rb");
  size_t retval = NU_Client_send_file(client, server, file, timeout);
  MU_ASSERT(retval, logger, "Was unable to send file to server!\n");
}

static void obtain_file(NU_Server_Socket_t *server, const char *msg){
  FILE *file = fopen(msg, "wb+");
  MU_ASSERT(file, logger, "Was unable to create file: \"%s\" with mode \"%s\"\n", msg, "wb+");
  size_t retval = NU_Client_receive_to_file(client, server, file, buffer_size, 1, timeout);
  MU_ASSERT(retval, logger, "Was unable to receive file from server!\n");
}
int main(void){
  size_t bytes_sent;
  char *client_inquiry;
  logger = calloc(1, sizeof(MU_Logger_t));
  MU_Logger_Init(logger, "NU_Client_Test_File_Downloader_Log.txt", "w", MU_ALL);
  client = NU_Client_create(0);
  MU_ASSERT(client, logger, "Was unable to create client!\n");
  char *ip_addr = calloc(1, INET_ADDRSTRLEN);
  MU_DEBUG("IP Address:");
  scanf("%s", ip_addr);
  NU_Server_Socket_t *server = NU_Client_connect(client, ip_addr, port_num, 0, timeout);
  MU_ASSERT(server, logger, "Failed while attempting to connect to server!");
  const char *msg = NU_Client_receive(client, server, buffer_size, timeout);
  MU_ASSERT(msg, logger, "Was unable to receive message from server!\n");
  if(strcmp(msg, "Filename...\n") == 0) send_file(server);
  else obtain_file(server, msg);
  free(ip_addr);
  NU_Client_destroy(client, "Shutting down!\n");
}