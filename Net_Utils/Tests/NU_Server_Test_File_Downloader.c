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
  size_t bytes_sent = NU_Server_send(server, client, (const char *)inquiry, strlen(inquiry), timeout);
  MU_ASSERT((bytes_sent == strlen(inquiry)), logger,
	    "Not all bytes were sent to client!\nbytes_sent: %zu, message_bytes: %zu\n", bytes_sent, strlen(inquiry));
  const char *retval = NU_Server_receive(server, client, buffer_size, timeout);
  MU_ASSERT(retval, logger, "Client either disconnected or did not send result in time!\n");
  return retval;
}

static void recv_file(NU_Client_Socket_t *client, const char *filename){
  FILE *file = fopen(filename, "wb");
  MU_ASSERT(file, logger, "Was unable to create file!\n");
  size_t retval = NU_Server_receive_to_file(server, client, file, buffer_size, 1, timeout);
  MU_DEBUG("Received %zu bytes into temporary file from client!\n", retval);
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
  MU_ASSERT(fgets(ip_addr, INET_ADDRSTRLEN, stdin), logger, "Invalid input from user!\n");
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, ip_addr, port_num, queue_max, NU_NONE);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  NU_Client_Socket_t *client = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client, logger, "Client did not connect in time!\n");
  MU_DEBUG("Client connected...\n");
  const char *filename = NU_Server_receive(server, client, buffer_size, timeout);
  MU_ASSERT(filename, logger, "Was unable to retrieve filename from client!\n");
  MU_DEBUG("Received filename: \"%s\"\n", filename);
  recv_file(client, filename);
  NU_Server_destroy(server, "Shutting down!\n");
}