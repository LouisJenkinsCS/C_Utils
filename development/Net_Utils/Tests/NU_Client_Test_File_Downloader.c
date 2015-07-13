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
  size_t bytes_sent = NU_Client_send(client, server, (const char *)inquiry, strlen(inquiry), timeout);
  MU_ASSERT(bytes_sent, logger, "Was unable to send information to server!\n");
  char *response = calloc(buffer_size + 1);
  size_t bytes_received = NU_Client_receive(client, server, response, buffer_size, timeout);
  MU_ASSERT(bytes_received, logger, "Was unable to get response from server!\n");
  return response;
}

static void send_file(NU_Server_Socket_t *server){
  char *filepath;
  char *filename = "";
  asprintf(&filepath, "%s/%s/%s", getenv("HOME"), "Pictures", filename);
  MU_DEBUG("Opening \"%s\"\n", filepath);
  FILE *file = fopen(filepath, "rb");
  MU_ASSERT(file, logger, "Was unable to open file: \"%s\" with mode \"%s\"!\nfopen: \"%s\"\n", filepath, "rb", strerror(errno));
  size_t sent = NU_Client_send(client, server, filename, strlen(filename), timeout);
  MU_DEBUG("Sent filename \"%s\"", filename);
  MU_ASSERT(sent, logger, "Was unable to send filename to server!\n");
  //MU_DEBUG("File Path:");
  //MU_ASSERT(fgets(filepath, 255, stdin), logger, "Invalid input from user!\n");
  sent = NU_Client_send_file(client, server, file, buffer_size, timeout);
  MU_DEBUG("Sent %zu bytes to server!\n", retval);
  MU_ASSERT(retval, logger, "Was unable to send file to server!\n");
}

int main(void){
  size_t bytes_sent;
  char *client_inquiry;
  logger = calloc(1, sizeof(MU_Logger_t));
  MU_Logger_Init(logger, "NU_Client_Test_File_Downloader_Log.txt", "w", MU_ALL);
  client = NU_Client_create(1);
  MU_ASSERT(client, logger, "Was unable to create client!\n");
  //char *ip_addr = calloc(1, INET_ADDRSTRLEN);
  //MU_DEBUG("IP Address:");
  //MU_ASSERT(fgets(ip_addr, INET_ADDRSTRLEN, stdin), logger, "Invalid input from user!\n");
  NU_Server_Socket_t *server = NU_Client_connect(client, "192.168.1.112", port_num, 0, timeout);
  MU_ASSERT(server, logger, "Failed while attempting to connect to server!");
  //send_file(server);
  //free(ip_addr);
  char *filename = "kitten.jpg";
  char *mode = "wb";
  FILE *file = fopen(filename, mode);
  MU_ASSERT(file, logger, "Was unable to create file \"%s\" with mode \"%s\"\nfopen: \"%s\"\n", filename, mode, strerror(errno)); 
  size_t received = NU_Client_receive_to_file(client, server, file, buffer_size, 1, timeout);
  MU_ASSERT(received, logger, "Client was unable to receive file from server!\n");
  NU_Client_destroy(client);
}
