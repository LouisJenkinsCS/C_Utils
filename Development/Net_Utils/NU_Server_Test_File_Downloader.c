#include <NU_Server.h>

MU_Logger_t *logger = NULL;
NU_Server_t *server = NULL;

int main(void){
  logger = calloc(1, sizeof(MU_Logger_t));
  MU_Logger_Init(logger, "NU_Server_Test_Log.txt", "w", MU_ALL);
  server = NU_Server_create(0);
  NU_Bound_Socket_t *bsock = NU_Server_bind(server, "10.0.2.15", port_num, queue_max, NU_NONE);
  MU_ASSERT(bsock, logger, "Failed while attempting to bind a socket!");
  pthread_t thread_one, thread_two;
}