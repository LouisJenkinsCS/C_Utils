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
static const int recv_flags = 0;
static const int send_flags = 0;

int main(void){
  logger = MU_Logger_create("./Net_Utils/Logs/NU_Server_File_Downloader.log", "w", MU_ALL);
  server = NU_Server_create(queue_max, max_sockets, is_threaded);
  MU_ASSERT(server, logger, "Was unable to create server!");
  char ip_addr[INET_ADDRSTRLEN];
  MU_DEBUG("IP Address:");
  MU_ASSERT(fgets(ip_addr, INET_ADDRSTRLEN, stdin), logger, "Invalid input from user!");
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, queue_max, 80, ip_addr);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  NU_Connection_t *client = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client, logger, "Client did not connect in time!");
  MU_DEBUG("Client connected...");
  //const char *filename = NU_Server_receive(server, client, buffer_size, timeout);
  //MU_ASSERT(filename, logger, "Was unable to retrieve filename from client!\n");
  //MU_DEBUG("Received filename: '%s'\n", filename);
  //recv_file(client, filename);
  char buf[buffer_size];
  size_t received = NU_Connection_receive(client, buf, buffer_size, timeout, recv_flags);
  MU_DEBUG("Received %zu bytes as header!\n%.*s", received, (int)received, buf);
  char *filepath = "C:/Users/theif519/Documents/theif519.html";
  MU_DEBUG("Opening '%s'", filepath);
  FILE *file = fopen(filepath, "rb");
  MU_ASSERT(file, logger, "fopen: '%s'", strerror(errno));
  struct stat file_stats; 
  int file_fd = fileno(file);
  MU_ASSERT((file_fd != -1), logger, "fileno: '%s'", strerror(errno));
  MU_ASSERT((fstat(file_fd, &file_stats) != -1), logger, "fstat: '%s'\n", strerror(errno));
  size_t file_size = file_stats.st_size;
  char *header;
  asprintf(&header, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %zu\r\n\r\n", file_size);
  size_t sent = NU_Connection_send(client, header, strlen(header), timeout, send_flags);
  MU_ASSERT(sent, logger, "Was unable to send header to client!");
  sent = NU_Connection_send_file(client, file, timeout, send_flags);
  MU_ASSERT(sent, logger, "Was unable to send data to client!");
  MU_DEBUG("Sent %zu bytes to client!", sent);
  /// Note that I lose reference to original bound socket, but it still gets unbound at end since Server keep pool of them.
  NU_Server_log(server, "Binding second port!");
  MU_DEBUG("Unbinding from port 80!");
  NU_Server_unbind(server, bsock);
  MU_DEBUG("Binding to port 10000...");
  bsock = NU_Server_bind(server, queue_max, 10000, "192.168.1.112");
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  client = NU_Server_accept(server, bsock, timeout);
  MU_ASSERT(client, logger, "Client did not connect in time!");
  MU_DEBUG("Client connected!");
  received = NU_Connection_receive(client, buf, buffer_size, timeout, recv_flags);
  MU_DEBUG("Received %zu bytes as header!\n%.*s", received, (int)received, buf);
  NU_Connection_send(client, header, strlen(header), timeout, send_flags);
  NU_Connection_send_file(client, file, timeout, send_flags);
  NU_Server_destroy(server);
  MU_Logger_destroy(logger);
  fclose(file);
  free(header);
  return EXIT_SUCCESS;
}
