#include <DS_Iterator.h>
#include <MU_Arg_Check.h>

void *DS_Iterator_next(DS_Iterator_t *it){
	if(!it || !it->next) return NULL;
	void *item;
	it->curr = it->next(it->ds_handle, it->curr, &item);
	return item;
}

void *DS_Iterator_prev(DS_Iterator_t *it){
	if(!it || !it->prev) return NULL;
	void *item;
	it->curr = it->prev(it->ds_handle, it->curr, &item);
	return item;
}

bool DS_Iterator_append(DS_Iterator_t *it){
	if(!it || !it->append) return false;
	bool result;
	it->curr = it->append(it->ds_handle, it->curr, &result);
	return result;
}

bool DS_Iterator_prepend(DS_Iterator_t *it){
	if(!it || !it->prepend) return false;
	bool result;
	it->curr = it->prepend(it->ds_handle, it->curr, &result);
	return result;
}

bool DS_Iterator_for_each(DS_Iterator_t *it, DS_general_cb cb){
	if(!it || !it->for_each) return false;
	bool result;
	it->curr = it->for_each(it->ds_handle, it->curr, cb, &result);
	return result;
}

bool DS_Iterator_remove(DS_Iterator_t *it, DS_delete_cb del){
	if(!it || !it->del) return NULL;
	bool result;
	it->curr = it->del(it->ds_handle, it->curr, del, &result);
	return result;
}