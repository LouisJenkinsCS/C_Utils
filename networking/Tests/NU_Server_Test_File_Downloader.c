#include <MU_Logger.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>
#include <NU_Server.h>
#include <NU_HTTP.h>
#include <TP_Pool.h>
#include <unistd.h>

static struct c_utils_logger *logger = NULL;
static NU_Server_t *server = NULL;
static TP_Pool_t *tp = NULL;
static const unsigned int timeout = 60;
static const unsigned int port_num = 10000;
static const unsigned int second_port_num = 80;
static const unsigned int queue_max = 10;
static const size_t worker_threads = 10;
static const unsigned int max_sockets = 2;
static const unsigned char is_threaded = 1;
static const char *ip_addr = "192.168.1.112";
static const int recv_flags = 0;
static const int send_flags = 0;
static const char *filepath = "Net_Utils/Misc/Web_Pages";

static FILE *open_web_page(const char *page){
  char buf[NU_HTTP_FILE_PATH_LEN + 1];
  snprintf(buf, NU_HTTP_FILE_PATH_LEN + 1, "%s%s", filepath, page);
  FILE *file = fopen(buf, "rb");
  return file;
}

static char *get_page_size(FILE *file){
  const int page_size_len = 32;
  char *buf = malloc(page_size_len + 1);
  MU_ASSERT(buf, logger, "Was unable to allocate buffer for page size!");
  struct stat file_stats; 
  int file_fd = fileno(file);
  MU_ASSERT((file_fd != -1), logger, "fileno: '%s'", strerror(errno));
  MU_ASSERT((fstat(file_fd, &file_stats) != -1), logger, "fstat: '%s'\n", strerror(errno));
  size_t file_size = file_stats.st_size;
  snprintf(buf, page_size_len + 1, "%zu", file_size);
  return buf;
}

static void *handle_connection(void *args){
  NU_Connection_t *conn = args;
  char buf[BUFSIZ + 1];
  size_t received = NU_Connection_receive(conn, buf, BUFSIZ, timeout, recv_flags);
  if(!received){
    MU_DEBUG("Unable to receive request");
    return NULL;
  }
  NU_Request_t *req = NU_Request_create();
  MU_ASSERT(req, logger, "Was unable to allocate memory for HTTP request!");
  char *retval = NU_Request_append_header(req, buf, &received);
  MU_DEBUG("Leftovers: '%.*s'", (int)received, retval);
  retval = NU_Request_to_string(req);
  MU_DEBUG("Received request:\n%s\n", retval);
  char *content_type;
  if(strncmp(req->path, "/styles", strlen("/styles")) == 0){
    content_type = "text/css; charset=UTF-8";
  } else content_type = "text/html; charset=UTF-8";
  // When HTTP is implemented, I will handle HTTP requests dynamically. For now I just return a consistent header.
  unsigned int status = 200;
  FILE *file = open_web_page(req->path);
  if(!file){
    file = open_web_page("/404.html");
    status = 400;
    MU_ASSERT(file, logger, "Missing vital core web pages!");
  }
  NU_Response_t *res = NU_Response_create();
  NU_RESPONSE_WRITE(res, status, NU_HTTP_VER_1_0, { "Content-Length", get_page_size(file) }, { "Content-Type", content_type });
  retval = NU_Response_to_string(res);
  size_t sent = NU_Connection_send(conn, retval, strlen(retval), timeout, send_flags);
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
  // Notice, we do not have to maintain a reference to the bound socket as server manages them for us.
  MU_ASSERT(NU_Server_bind(server, queue_max, port_num, ip_addr), logger, "Failed while attempting to bind a socket!");
  MU_ASSERT(NU_Server_bind(server, queue_max, second_port_num, ip_addr), logger, "Failed while attempting to bind a socket!");
  while(1){
    NU_Connection_t *client = NU_Server_accept_any(server, -1); 
    MU_DEBUG("Client connected...");
    TP_Pool_add(tp, handle_connection, client, TP_NO_RESULT);
  }  
  NU_Server_destroy(server);
  MU_Logger_destroy(logger);
  return EXIT_SUCCESS;
}
