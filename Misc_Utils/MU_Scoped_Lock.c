#include <MU_Scoped_Lock.h>
#include <MU_Logger.h>

static MU_Logger_t *logger = NULL;

MU_LOGGER_AUTO_CREATE(logger, "./Logs/MU_Scoped_Lock.log", "w", MU_ALL)

void auto_unlock(MU_Scoped_Lock_t **lock){
	(*lock)->unlock((*lock)->instance);
}