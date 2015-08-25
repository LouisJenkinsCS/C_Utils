#include <MU_Hazard_Pointers.h>
#include <stdlib.h>

static MU_Logger_t *logger = NULL;

static const int max_hazard_pointers = MU_HAZARD_POINTERS_MAX_THREADS * MU_HAZARD_POINTERS_PER_THREAD;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Misc_Utils/Logs/MU_Event_Loop.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

static void scan(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp){
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
			list->destructor(tmp_arr[i]);
		}
		free(tmp_arr);
	}
}

static void retire(MU_Hazard_Pointer_t *hp){
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		hp->hazard_pointers[i] = NULL;
	}
	hp->in_use = false;
}

static void help_scan(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp){
	for(MU_Hazard_Pointer_t *tmp_hp = list->head; tmp_hp; tmp_hp = tmp_hp->next){
		// If we fail to mark the hazard pointer as active, then it's already in use.
		bool expected = false;
		if(atomic_load(&tmp_hp->in_use) || atomic_compare_exchange_strong(&tmp_hp->in_use, &expected, true)) continue; 
		void *data;
		while((data = DS_List_remove_at(tmp_hp->retired, 0, NULL))){
			DS_List_add(hp->retired, data, NULL);
			if(hp->retired->size >= max_hazard_pointers) scan(list, hp);
		}
		atomic_store(&tmp_hp->in_use, false);
	}
}

static MU_Hazard_Pointer_t *create(){
	MU_Hazard_Pointer_t *hp = calloc(1, sizeof(*hp));
	if(!hp){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	hp->in_use = true;
	hp->retired = DS_List_create(false);
	if(!hp->retired){
		MU_LOG_ERROR(logger, "DS_List_create: '%s'", strerror(errno));
		free(hp);
		return NULL;
	}
	return hp;
}

MU_Hazard_Pointer_List_t *MU_Hazard_Pointer_init(void (*destructor)(void *)){
	MU_Hazard_Pointer_List_t *list = calloc(1, sizeof(*list));
	if(!list){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
	}
	list->destructor = destructor;
	return list;
}

MU_Hazard_Pointer_t *MU_Hazard_Pointer_get(MU_Hazard_Pointer_List_t *list){
	for(MU_Hazard_Pointer_t *tmp_hp = list->head; tmp_hp; tmp_hp = tmp_hp->next){
		bool expected = false;
		if(atomic_load(&tmp_hp->in_use) || atomic_compare_exchange_strong(&tmp_hp->in_use, &expected, true)) continue;
		else return tmp_hp;
	}
	atomic_fetch_add(&list->size, MU_HAZARD_POINTERS_PER_THREAD);
	MU_Hazard_Pointer_t *hp = create();
	if(!hp){
		MU_LOG_ERROR(logger, "create_hp: 'Was unable to allocate a Hazard Pointer!");
		return NULL;
	}
	MU_Hazard_Pointer_t *old_head;
	do {
		old_head = list->head;
		hp->next = old_head;
	} while(!__sync_bool_compare_and_swap(&list->head, old_head, hp));
	return hp;
}

void MU_Hazard_Pointer_release(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp, void *data){
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		if(hp->hazard_pointers[i] == data){
			DS_List_add(hp->retired, data, NULL);
			hp->hazard_pointers[i] = NULL;
			if(hp->retired->size >= max_hazard_pointers){
				scan(list, hp);
				help_scan(list, hp);
			}
		}
	}
}

void MU_Hazard_Pointer_release_all(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp){
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		if(hp->hazard_pointers[i]){
			DS_List_add(hp->retired, hp->hazard_pointers[i], NULL);
			hp->hazard_pointers[i] = NULL;
			if(hp->retired->size >= max_hazard_pointers){
				scan(list, hp);
				help_scan(list, hp);
			}
		}
	}
}

void MU_Hazard_Pointer_acquire(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp, void *data){
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		if(!hp->hazard_pointers[i]){
			hp->hazard_pointers[i] = data;
		}
	}
}