#define NO_C_UTILS_PREFIX
#include "../server.h"
#include "../../io/logger.h"
#include <stdlib.h>

/*
	This test shows off the simplicity of this library. If you remove safety checks, it comes down to...

	server_t *server = server_create(...);
	socket_t *sock = server_bind(server, ...);
	connection_t *conn = server_accept(server, sock, ...);
	connection_send_file(conn, ...);
*/

static logger_t *logger;

LOGGER_AUTO_CREATE(logger, "./networking/logs/server_file_downloader_test.log", "w", LOG_LEVEL_ALL);

static const char *ip_addr = "192.168.1.136";

static void send_file(connection_t *conn) {
  char *filepath;
  char *filename = "kitten.jpg";
  asprintf(&filepath, "%s/%s/%s", getenv("HOME"), "Pictures", filename);
  
  DEBUG("Opening \"%s\"\n", filepath);
  FILE *file = fopen(filepath, "rb");
  ASSERT(file, logger, "Was unable to open file: \"%s\" with mode \"%s\"!\nfopen: \"%s\"\n", filepath, "rb", strerror(errno));
  
  size_t sent = connection_send_file(conn, file, BUFSIZ, -1);
  DEBUG("Sent %zu bytes to server!\n", sent);
  ASSERT(sent, logger, "Was unable to send file to server!\n");

  fclose(file);
}

int main(void) {
	// 1 Connection, 1 Socket, no synchronicity.
	server_t *server = server_create(1, 1, false);
	assert(server);

	socket_t *sock = server_bind(server, 1, 10000, ip_addr);
	assert(sock);

	connection_t *conn = server_accept(server, sock, -1);
	assert(conn);

	send_file(conn);
}