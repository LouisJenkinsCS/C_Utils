#define NO_C_UTILS_PREFIX
#include "../../io/logger.h"
#include "../client.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static const char *ip_addr = "192.168.1.136";
static struct c_utils_logger *logger = NULL;
static client_t *client = NULL;
static const unsigned int timeout = 60;
static const size_t buffer_size = 1024;
static const unsigned int port_num = 10000;
static const bool is_threaded = true;
static const unsigned int conn_pool_size = 1;

static const char *assert_get_info(connection_t *conn, const char *inquiry) {
  size_t bytes_sent = connection_send(conn, (const char *)inquiry, strlen(inquiry), timeout, 0);
  ASSERT(bytes_sent, logger, "Was unable to send information to server!\n");
  
  char *response = calloc(1, buffer_size + 1);
  assert(response);

  size_t bytes_received = connection_receive(conn, response, buffer_size, timeout, 0);
  ASSERT(bytes_received, logger, "Was unable to get response from server!\n");

  return response;
}

static void send_file(connection_t *conn) {
  char *filepath;
  char *filename = "";
  asprintf(&filepath, "%s/%s/%s", getenv("HOME"), "Pictures", filename);
  
  DEBUG("Opening \"%s\"\n", filepath);
  FILE *file = fopen(filepath, "rb");
  ASSERT(file, logger, "Was unable to open file: \"%s\" with mode \"%s\"!\nfopen: \"%s\"\n", filepath, "rb", strerror(errno));
  
  size_t sent = connection_send(conn, filename, strlen(filename), timeout, 0);
  DEBUG("Sent filename \"%s\"", filename);
  ASSERT(sent, logger, "Was unable to send filename to server!\n");
  
  sent = connection_send_file(conn, file, buffer_size, timeout);
  DEBUG("Sent %zu bytes to server!\n", sent);
  ASSERT(sent, logger, "Was unable to send file to server!\n");
}

int main(void) {
  logger = logger_create("./networking/logs/client_file_downloader_test.log", "w", LOG_LEVEL_ALL);
  client = client_create(conn_pool_size, is_threaded);
  ASSERT(client, logger, "Was unable to create client!\n");

  connection_t *conn = client_connect(client, ip_addr, port_num, timeout);
  ASSERT(conn, logger, "Failed while attempting to connect to end-point!");

  char *filename = "kitten.jpg";
  char *mode = "wb";
  FILE *file = fopen(filename, mode);
  ASSERT(file, logger, "Was unable to create file \"%s\" with mode \"%s\"\nfopen: \"%s\"\n", filename, mode, strerror(errno)); 
  
  size_t received = connection_receive_file(conn, file, buffer_size, timeout);
  ASSERT(received, logger, "Client was unable to receive file from server!\n");
  client_destroy(client);
}
