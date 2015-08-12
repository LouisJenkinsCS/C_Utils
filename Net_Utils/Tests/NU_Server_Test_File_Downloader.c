#include <MU_Logger.h>
#include <pthread.h>
#include <stdio.h>
#include <NU_Server.h>
#include <TP_Pool.h>
#include <unistd.h>

static MU_Logger_t *logger = NULL;
static NU_Server_t *server = NULL;
static TP_Pool_t *tp = NULL;
static const unsigned int timeout = 60;
static const unsigned int port_num = 10000;
static const unsigned int second_port_num = 80;
static const unsigned int queue_max = 1;
static const size_t worker_threads = 10;
static const unsigned int max_sockets = 1;
static const unsigned char is_threaded = 1;
static const char *ip_addr = "192.168.1.112";
static const int recv_flags = 0;
static const int send_flags = 0;
static const char *filepath = "C:/Users/theif519/Documents/theif519.html";

static void *handle_connection(void *args){
  NU_Connection_t *conn = args;
  char buf[BUFSIZ + 1];
  char header[BUFSIZ + 1];
  size_t received = NU_Connection_receive(conn, buf, BUFSIZ, timeout, recv_flags);
  if(!received){
    MU_DEBUG("Unable to receive request");
    return NULL;
  }
  MU_DEBUG("Received request:\n%s\n", buf);
  // When HTTP is implemented, I will handle HTTP requests dynamically. For now I just return a consistent header.
  FILE *file = fopen(filepath, "rb");
  MU_ASSERT(file, logger, "fopen: '%s'", strerror(errno));
  struct stat file_stats; 
  int file_fd = fileno(file);
  MU_ASSERT((file_fd != -1), logger, "fileno: '%s'", strerror(errno));
  MU_ASSERT((fstat(file_fd, &file_stats) != -1), logger, "fstat: '%s'\n", strerror(errno));
  size_t file_size = file_stats.st_size;
  sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %zu\r\n\r\n", file_size);
  size_t sent = NU_Connection_send(conn, header, strlen(header), timeout, send_flags);
  if(!sent){
    MU_DEBUG("Unable to send response!");
    fclose(file);
    return NULL;
  }
  MU_DEBUG("Sent response!");
  sent = NU_Connection_send_file(conn, file, timeout, send_flags);
  if(!sent){
    fclose(file);
    MU_DEBUG("Unable to send file!");
    return NULL;
  }
  MU_DEBUG("Sent file!");
  fclose(file);
  NU_Server_disconnect(server, conn);
  MU_DEBUG("Disconnected!");
  return NULL;
}

int main(void){
  logger = MU_Logger_create("./Net_Utils/Logs/NU_Server_File_Downloader.log", "w", MU_ALL);
  server = NU_Server_create(queue_max, max_sockets, is_threaded);
  MU_ASSERT(server, logger, "Was unable to create server!");
  tp = TP_Pool_create(worker_threads);
  MU_ASSERT(tp, logger, "Was unable to create thread pool!");
  NU_Bound_Socket_t *bsock_one = NU_Server_bind(server, queue_max, port_num, ip_addr);
  MU_ASSERT(bsock_one, logger, "Failed while attempting to bind a socket!");
  NU_Bound_Socket_t *bsock_two = NU_Server_bind(server, queue_max, second_port_num, ip_addr);
  MU_ASSERT(bsock_two, logger, "Failed while attempting to bind a socket!");
  NU_Connection_t *client;
  while(1){
    if((client = NU_Server_accept(server, bsock_one, 0)) || (client = NU_Server_accept(server, bsock_two, 0))){
      MU_DEBUG("Client connected...");
      TP_Pool_add(tp, handle_connection, client, TP_NO_RESULT);
    }
    sleep(1);
  }  
  NU_Server_destroy(server);
  MU_Logger_destroy(logger);
  return EXIT_SUCCESS;
}
