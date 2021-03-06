#include "iterator.h"
#include "../memory/ref_count.h"

void c_utils_auto_destroy_iterator(struct c_utils_iterator **it){
	c_utils_iterator_destroy(*it);
}

void *c_utils_iterator_head(struct c_utils_iterator *it){
	if (!it || !it->head)
		return NULL;

	return it->head(it->handle, it->pos);
}

void *c_utils_iterator_tail(struct c_utils_iterator *it){
	if (!it || !it->tail)
		return NULL;

	return it->tail(it->handle, it->pos);
}

void *c_utils_iterator_next(struct c_utils_iterator *it){
	if (!it || !it->next)
		return NULL;

	return it->next(it->handle, it->pos);
}

void *c_utils_iterator_prev(struct c_utils_iterator *it){
	if (!it || !it->prev)
		return NULL;
	
	return it->prev(it->handle, it->pos);
}

void *c_utils_iterator_curr(struct c_utils_iterator *it) {
	if(!it || !it->curr)
		return NULL;

	return it->curr(it->handle, it->pos);
}

bool c_utils_iterator_append(struct c_utils_iterator *it, void *item){
	if (!it || !it->append)
		return false;

	return it->append(it->handle, it->pos, item);
}

bool c_utils_iterator_prepend(struct c_utils_iterator *it, void *item){
	if (!it || !it->prepend)
		return false;

	return it->prepend(it->handle, it->pos, item);
}

bool c_utils_iterator_for_each(struct c_utils_iterator *it, c_utils_general_cb cb){
	if (!it || !it->for_each)
		return false;

	return it->for_each(it->handle, it->pos, cb);
}

bool c_utils_iterator_remove(struct c_utils_iterator *it) {
	if (!it || !it->rem)
		return false;

	return it->rem(it->handle, it->pos);
}

bool c_utils_iterator_delete(struct c_utils_iterator *it) {
	if (!it || !it->del)
		return false;

	return it->del(it->handle, it->pos);
}

void c_utils_iterator_destroy(struct c_utils_iterator *it) {
	if(!it)
		return;

	if(it->conf.ref_counted)
		C_UTILS_REF_DEC(it->handle);

	if(it->finalize)
		it->finalize(it->handle, it->pos);

	free(it);
}