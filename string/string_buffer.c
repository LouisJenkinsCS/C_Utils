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
static const int default_size = 64;
static struct c_utils_logger *logger;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./string/logs/string_buffer.log", "w", C_UTILS_LOG_LEVEL_INFO);

static inline bool resize(struct c_utils_string_buffer *buf, int size) {
	int new_size = size * resize_ratio;

	if(buf->allocated >= new_size)
		return false;

	C_UTILS_ON_BAD_REALLOC((&buf->data), logger, new_size)
		return false;

	buf->allocated = new_size;

	return true;
}

static inline bool clear_buf(struct c_utils_string_buffer *buf) {
	resize(buf, default_size);
	buf->used = 0;

	return true;
}

static inline char *get_data(struct c_utils_string_buffer *buf) {
	char *str;
	C_UTILS_ON_BAD_MALLOC(str, logger, buf->used)
			return NULL;

	snprintf(str, buf->used, "%s", buf->data);

	return str;
}

struct c_utils_string_buffer *c_utils_string_buffer_create(char *str, bool synchronized) {
	struct c_utils_string_buffer *buf;
	C_UTILS_ON_BAD_CALLOC(buf, logger, sizeof(*buf))
		goto err;

	int len = (str ? strlen(str) * resize_ratio : default_size) + 1;
	C_UTILS_ON_BAD_MALLOC(buf->data, logger, len) {
		goto err_data;
	}

	if(str)
		snprintf(buf->data, len, "%s", str);

	buf->used = str ? len : 1;
	buf->allocated = len;

	buf->lock = synchronized ? c_utils_scoped_lock_spinlock(0, logger) : c_utils_scoped_lock_no_op();
	if(!buf->lock)
		goto err_lock;

	return buf;

	err_lock:
		free(buf->data);
	err_data:
		free(buf);
	err:
		return NULL;
}

bool c_utils_string_buffer_append(struct c_utils_string_buffer *buf, char *str) {
	C_UTILS_ARG_CHECK(logger, false, buf, str);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		int len = strlen(str) + buf->used + 1;
		if(len >= buf->allocatated)
			if(!resize(buf, len))
				return false;

		snprintf(buf->data, len, "%s%s", buf->data, str);
	}

	return true;
}

bool c_utils_string_buffer_prepend(struct c_utils_string_buffer *buf, char *str) {
	C_UTILS_ARG_CHECK(logger, false, buf, str);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		int len = strlen(str) + buf->used + 1;
		if(len >= buf->allocatated)
			if(!resize(buf, len))
				return false;

		snprintf(buf->data, len, "%s%s", str, buf->data);
	}

	return true;
}

bool c_utils_string_buffer_insert(struct c_utils_string_buffer *buf, char *str, int index) {
	C_UTILS_ARG_CHECK(logger, false, buf, str);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		index = index < 0 ? buf->used + index : index;
		if(index < 0) {
			C_UTILS_LOG_ERROR("Index is out of bounds!");
			return NULL;
		}

		int len = buf->used + strlen(str) + 1;
		if(len >= buf->allocated)
			if(!resize(buf, len))
				return false;

		snprintf(buf->data, len, "%.*s%s%s", index, buf->data, str, buf->data + index);
	}

	return true;

}

bool c_utils_string_buffer_reverse(struct c_utils_string_buffer *buf) {
	C_UTILS_ARG_CHECK(logger, false, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		int i = 0, j = buf->used - 1;
		for(int i = 0; j > i; i++, j--) {
			char c = buf->used[i];
			buf->used[i] = buf->used[j];
			buf->used[j] = c;
		}
	}

	return true;
}

int c_utils_string_buffer_size(struct c_utils_string_buffer *buf) {
	C_UTILS_ARG_CHECK(logger, -1, buf);

	C_UTILS_SCOPED_LOCK(buf->lock)
		return buf->used - 1;

	C_UTILS_UNACCESSIBLE;
}

bool c_utils_string_buffer_delete(struct c_utils_string_buffer *buf, int start, int end) {
	C_UTILS_ARG_CHECK(logger, false, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		start = start < 0 ? buf->used + start : start;
		if(start < 0) {
			C_UTILS_LOG_ERROR("Start index is out of bounds!");
			return NULL;
		}

		end = end < 0 ? buf->used + end : end;
		if(end < 0) {
			C_UTILS_LOG_ERROR("End index is out of bounds");
			return NULL;
		} else if (start > end) {
			C_UTILS_LOG_ERROR("Start index > End index");
			return NULL;
		}

		int len = buf->used - (end - start) + 1;
		buf->used = len;

		snprintf(buf->data, len, "%.*s%s", start, buf->data, buf->data + end);

		resize(buf, len);
	}

	return true;
}

bool c_utils_string_buffer_clear(struct c_utils_string_buffer *buf) {
	C_UTILS_ARG_CHECK(logger, false, buf);

	C_UTILS_SCOPED_LOCK(buf->lock)
		clear_buf(buf);

	return true;
}

char *c_utils_string_buffer_get(struct c_utils_string_buffer *buf) {
	C_UTILS_ARG_CHECK(logger, NULL, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		return get_data(buf);
	}

	C_UTILS_UNACCESSIBLE;
}

char *c_utils_string_buffer_take(struct c_utils_string_buffer *buf) {
	C_UTILS_ARG_CHECK(logger, NULL, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		char *str = get_data(buf);
		clear_buf(buf);
		return str;
	}

	C_UTILS_UNACCESSIBLE;
}

char *c_utils_string_buffer_substring(struct c_utils_string_buffer *buf, int start, int end) {
	C_UTILS_ARG_CHECK(logger, NULL, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		start = start < 0 ? buf->used + start : start;
		if(start < 0) {
			C_UTILS_LOG_ERROR("Start index is out of bounds!");
			return NULL;
		}

		end = end < 0 ? buf->used + end : end;
		if(end < 0) {
			C_UTILS_LOG_ERROR("End index is out of bounds");
			return NULL;
		} else if (start > end) {
			C_UTILS_LOG_ERROR("Start index > End index");
			return NULL;
		}

		int len = (end - start) + 1;
		if(len >= used)
			return get_data(buf);

		char *str;
		C_UTILS_ON_BAD_MALLOC(str, logger, len)
			return NULL;

		snprintf(str, len, "%.*s", buf->data + start);

		return str;
	}

	C_UTILS_UNACCESSIBLE;
}

char *c_utils_string_buffer_beyond(struct c_utils_string_buffer *buf, int after) {
	C_UTILS_ARG_CHECK(logger, NULL, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		if(!after)
			return get_data(buf);

		after = after < 0 ? buf->used + after : after;
		if(after < 0) {
			C_UTILS_LOG_ERROR("After index is out of bounds!");
			return NULL;
		}

		int len = (buf->used - after) + 1;

		char *str;
		C_UTILS_ON_BAD_MALLOC(str, logger, len)
			return NULL;

		snprintf(str, len, "%.*s", buf->data + after);

		return str;
	}

	C_UTILS_UNACCESSIBLE;
}

char *c_utils_string_buffer_before(struct c_utils_string_buffer *buf, int before) {
	C_UTILS_ARG_CHECK(logger, NULL, buf);

	C_UTILS_SCOPED_LOCK(buf->lock) {
		if(!before)
			return NULL;

		before = before < 0 ? buf->used + before : before;
		if(before < 0) {
			C_UTILS_LOG_ERROR("After index is out of bounds!");
			return NULL;
		}

		int len = before + 1;

		char *str;
		C_UTILS_ON_BAD_MALLOC(str, logger, len)
			return NULL;

		snprintf(str, len, "%s", buf->data);

		return str;
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_string_buffer_destroy(struct c_utils_string_buffer *buf) {
	if(!buf)
		return;

	free(buf->data);
	c_utils_scoped_lock_destroy(buf->lock);
	free(buf);
}