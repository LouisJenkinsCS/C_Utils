#include <NU_HTTP.h>
#include <NU_Server.h>

#include <MU_Logger.h>
#include <pthread.h>
#include <stdio.h>
#include <NU_Server.h>
#include <unistd.h>

static struct c_utils_logger *logger = NULL;
static NU_Server_t *server = NULL;
static const unsigned int timeout = 60;
static const size_t buffer_size = 1024;
static const unsigned int port_num = 10000;
static const unsigned int queue_max = 1;
static const unsigned int max_sockets = 1;
static const unsigned char is_threaded = 1;

int main(void) {
  logger = MU_Logger_create("NU_HTTP_Header_Test.log", "w", MU_ALL);
  server = NU_Server_create(queue_max, max_sockets, is_threaded);
  MU_ASSERT(server, logger, "Was unable to create server!");
  char ip_addr[INET_ADDRSTRLEN];
  MU_DEBUG("IP Address:");
  MU_ASSERT(fgets(ip_addr, INET_ADDRSTRLEN, stdin), logger, "Invalid input from user!");
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, queue_max, port_num, ip_addr);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  NU_Connection_t *client = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client, logger, "Client did not connect in time!");
  MU_DEBUG("Client connected...");
  char *header_str = malloc(buffer_size);
  int received = NU_Server_receive(server, client, header_str, buffer_size, timeout);
  MU_ASSERT(received, logger, "NU_Server_receive: 'Was unable to receive header from client!'");
  NU_Header_t *request = NU_Header_from(header_str, received);
  MU_ASSERT(request, logger, "NU_Header_from: 'Was unable to create request header!");
  char *request_header_str = NU_Header_to_string(request);
  MU_DEBUG("NU_HTTP_to_string: \n%s", request_header_str);
  char *filepath = "C:/Users/theif519/Documents/theif519.html";
  MU_DEBUG("Opening '%s'", filepath);
  FILE *file = fopen(filepath, "rb");
  MU_ASSERT(file, logger, "fopen: '%s'", strerror(errno));
  struct stat file_stats; 
  int file_fd = fileno(file);
  MU_ASSERT((file_fd != -1), logger, "fileno: '%s'", strerror(errno));
  MU_ASSERT((fstat(file_fd, &file_stats) != -1), logger, "fstat: '%s'", strerror(errno));
  size_t file_size = file_stats.st_size;
  char *header;
  asprintf(&header, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %zu\r\n\r\n", file_size);
  size_t sent = NU_Server_send(server, client, header, strlen(header), timeout);
  free(header);
  MU_ASSERT(sent, logger, "Was unable to send header to client!");
  sent = NU_Server_send_file(server, client, file, buffer_size, timeout);
  fclose(file);
  MU_ASSERT(sent, logger, "Was unable to send data to client!");
  MU_DEBUG("Sent %zu bytes to client!", sent);
  char buf[buffer_size];
  received = NU_Server_receive(server, client, buf, buffer_size, timeout);
  MU_DEBUG("%.*s", received, buf);
  NU_Server_disconnect(server, client);
  NU_Server_destroy(server);
  MU_Logger_destroy(logger);
  free(logger);
  return EXIT_SUCCESS;
}
