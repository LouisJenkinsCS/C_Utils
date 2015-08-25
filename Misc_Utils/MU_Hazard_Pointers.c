#include <MU_Hazard_Pointers.h>
#include <stdlib.h>

static const int max_hazard_pointers = 64;

void MU_Hazard_Pointer_scan(MU_HP_List_t *list, MU_Hazard_Pointer_t *hp){
	DS_List_t *private_list = DS_List_create(false);
	/*
		Loop through the hazard pointers owned by each thread. Add them to the private if they
		are non-NULL, hence in use.
	*/
	for(MU_Hazard_Pointer_t *tmp_hp = list->head; tmp_hp; tmp_hp = tmp_hp->next){
		for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
			if(tmp_hp->hazard_pointers[i]){
				DS_List_add(private_list, tmp_hp->hazard_pointers[i], NULL);
			}
		}
	}
	size_t arr_size = 0;
	void **tmp_arr = DS_List_to_array(hp->retired, &arr_size);
	DS_List_clear(hp->retired, NULL);
	/*
		Since each thread has it's own list of retired pointers, we check to see
		if this thread's "retired" is no longer in the list of hazard pointers, hence
		no longer in use and can be freed.
	*/
	for(int i = 0; i < arr_size; i++){
		if(DS_List_contains(private_list, tmp_arr[i])){
			DS_List_add(hp->retired, tmp_arr[i], NULL);
		} else {
			free(tmp_arr[i]);
		}
		free(tmp_arr);
	}
}

void MU_Hazard_Pointer_help_scan(MU_HP_List_t *list, MU_Hazard_Pointer_t *hp){
	for(MU_Hazard_Pointer_t *tmp_hp = list->head; tmp_hp; tmp_hp = tmp_hp->next){
		// If we fail to mark the hazard pointer as active, then it's already in use.
		if(atomic_load(&tmp_hp->in_use) || atomic_compare_exchange_strong(&tmp_hp->in_use, false, true)) continue; 
		void *data;
		while((data = DS_List_remove_at(tmp_hp->retired, 0, NULL))){
			DS_List_add(hp->retired, data, NULL);
			if(hp->retired->size >= max_hazard_pointers) MU_Hazard_Pointer_scan(list, hp);
		}
		atomic_store(&tmp_hp->in_use, false);
	}
}

MU_HP_List_t *MU_HP_List_create();

MU_Hazard_Pointer_t *MU_Hazard_Pointer_create();

MU_Hazard_Pointer_t *MU_Hazard_Pointer_allocate(MU_HP_List_t *hp);

void MU_Hazard_Pointer_retire(MU_Hazard_Pointer_t *hp);

void MU_Hazard_Pointer_release(MU_HP_List_t *list, MU_Hazard_Pointer_t *hp, void *data);