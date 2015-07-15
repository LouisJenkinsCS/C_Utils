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
static const unsigned int queue_max = 1;
static const unsigned int max_sockets = 1;
static const unsigned char is_threaded = 1;

static const char *assert_get_info(NU_Connection_t *client, const char *inquiry){
  size_t bytes_sent = NU_Server_send(server, client, (const char *)inquiry, strlen(inquiry), timeout);
  MU_ASSERT(bytes_sent, logger, "Was unable to send information to client!\n");
  char *response = calloc(1, buffer_size + 1);
  size_t bytes_received = NU_Server_receive(server, client, response, buffer_size, timeout);
  MU_ASSERT(bytes_received, logger, "Was unable to get response from client!\n");
  return response;
}

static void recv_file(NU_Connection_t *client, const char *filename){
  FILE *file = fopen(filename, "wb");
  MU_ASSERT(file, logger, "Was unable to create file!fopen: \"%s\"\n", strerror(errno));
  size_t received = NU_Server_receive_file(server, client, file, buffer_size, timeout);
  MU_DEBUG("Received %zu bytes into temporary file from client!\n", received);
}

int main(void){
  size_t bytes_sent;
  char *client_inquiry;
  logger = calloc(1, sizeof(MU_Logger_t));
  MU_Logger_Init(logger, "NU_Server_File_Downloader.log", "w", MU_ALL);
  server = NU_Server_create(queue_max, max_sockets, is_threaded);
  MU_ASSERT(server, logger, "Was unable to create server!\n");
  char ip_addr[INET_ADDRSTRLEN];
  MU_DEBUG("IP Address:");
  MU_ASSERT(fgets(ip_addr, INET_ADDRSTRLEN, stdin), logger, "Invalid input from user!\n");
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, queue_max, port_num, ip_addr);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  NU_Connection_t *client = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client, logger, "Client did not connect in time!\n");
  MU_DEBUG("Client connected...\n");
  //const char *filename = NU_Server_receive(server, client, buffer_size, timeout);
  //MU_ASSERT(filename, logger, "Was unable to retrieve filename from client!\n");
  //MU_DEBUG("Received filename: \"%s\"\n", filename);
  //recv_file(client, filename);
  char *filepath = "/home/theif519/Pictures/index.html";
  MU_DEBUG("Opening \"%s\"\n", filepath);
  FILE *file = fopen(filepath, "rb");
  MU_ASSERT(file, logger, "Was unable to open file: \"%s\" with mode \"%s\"", filepath, "rb");
  size_t retval = NU_Server_send_file(server, client, file, buffer_size, timeout);
  MU_ASSERT(retval, logger, "Was unable to send data to client!\n");
  MU_DEBUG("Sent %zu bytes to client!\n", retval);
  NU_Server_destroy(server);
}
