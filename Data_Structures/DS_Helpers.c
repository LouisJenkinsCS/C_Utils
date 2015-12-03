#include <DS_Helpers.h>
#include <MU_Logger.h>

DS_Node_t *DS_Node_create(void *item, MU_Logger_t *logger){
	DS_Node_t *node = calloc(1, sizeof(*node));
	if(!node){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	node->item = item;
	return node;
}