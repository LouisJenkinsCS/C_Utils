#include "string_buffer.h"
#include "../threading/scoped_lock.h"
#include "../misc/alloc_check.h"

struct c_utils_string_buffer {
	char *data;
	int used;
	int allocated;
	struct c_utils_scoped_lock *lock;
};

static const float resize_ratio = 1.5;
static struct c_utils_logger *logger;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./string/logs/string_buffer.log", "w", C_UTILS_LOG_LEVEL_ALL);

static inline bool resize(struct c_utils_string_buffer *buf, int size) {
	int new_size = size * resize_ratio;

	if(buf->allocated >= new_size)
		return false;

	C_UTILS_ON_BAD_REALLOC((&buf->data), logger, new_size)
		return false;

	return true;
}