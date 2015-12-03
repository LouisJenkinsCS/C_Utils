#include <DS_Iterator.h>
#include <MU_Arg_Check.h>

void *DS_Iterator_head(DS_Iterator_t *it){
	if(!it || !it->head) return NULL;
	return it->head(it->ds_handle, &it->pos);
}

void *DS_Iterator_tail(DS_Iterator_t *it){
	if(!it || !it->tail) return NULL;
	return it->tail(it->ds_handle, &it->pos);
}

void *DS_Iterator_next(DS_Iterator_t *it){
	if(!it || !it->next) return NULL;
	return it->next(it->ds_handle, &it->pos);
}

void *DS_Iterator_prev(DS_Iterator_t *it){
	if(!it || !it->prev) return NULL;
	return it->prev(it->ds_handle, &it->pos);
}

bool DS_Iterator_append(DS_Iterator_t *it, void *item){
	if(!it || !it->append) return false;
	return it->append(it->ds_handle, &it->pos, item);
}

bool DS_Iterator_prepend(DS_Iterator_t *it, void *item){
	if(!it || !it->prepend) return false;
	return it->prepend(it->ds_handle, &it->pos, item);
}

bool DS_Iterator_for_each(DS_Iterator_t *it, DS_general_cb cb){
	if(!it || !it->for_each) return false;
	return it->for_each(it->ds_handle, &it->pos, cb);
}

bool DS_Iterator_remove(DS_Iterator_t *it, DS_delete_cb del){
	if(!it || !it->del) return NULL;
	return it->del(it->ds_handle, &it->pos, del);
}