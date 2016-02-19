#include <MU_Hazard_Pointers.h>
#include <MU_Arg_Check.h>
#include <stdlib.h>

typedef struct {
	MU_Hazard_Pointer_t *head;
	volatile size_t size;
	void (*destructor)(void *);
} MU_Hazard_Pointer_List_t;

static MU_Hazard_Pointer_List_t *hazard_table = NULL;

static pthread_key_t tls;

static MU_Logger_t *logger = NULL;

static const int max_hazard_pointers = MU_HAZARD_POINTERS_MAX_THREADS * MU_HAZARD_POINTERS_PER_THREAD;

<<<<<<< HEAD
__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Misc_Utils/Logs/MU_Hazard_Pointers.log", "w", MU_INFO);
}
=======
MU_LOGGER_AUTO_CREATE(logger, "./Misc_Utils/Logs/MU_Hazard_Pointers.log", "w", MU_INFO);
>>>>>>> development

__attribute__((constructor)) static void init_hazard_table(void){
	hazard_table = calloc(1, sizeof(*hazard_table));
	if(!hazard_table){
		MU_DEBUG("Was unable to allocate the hazard pointer table!");
		return;
	}
	hazard_table->destructor = free;
	hazard_table->size = 0;
}

__attribute__((constructor)) static void init_tls_key(void){
	pthread_key_create(&tls, NULL);
}

<<<<<<< HEAD
__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

=======
>>>>>>> development
/*
	TODO: Major, please optimize this thing, I can't even tell it's complexity. I think
	O(N^3)? Regardless, it's awfully inefficient, and a hash table would at least make it O(N).
*/
__attribute__((destructor)) static void destroy_hazard_table(void){
	MU_Hazard_Pointer_t *prev_hp = NULL;
	DS_List_t *del_list = DS_List_create(false);
	for(MU_Hazard_Pointer_t *hp = hazard_table->head; hp; hp = hp->next){
		free(prev_hp);
		DS_Iterator_t *it = DS_List_iterator(hp->retired);
		for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
			if(hp->owned[i] && !DS_List_contains(del_list, hp->owned[i])){
				DS_List_add(del_list, hp->owned[i], NULL);
			}
		}
		void *ptr;
		while((ptr = DS_Iterator_next(it))){
			if(!DS_List_contains(del_list, ptr)){
				DS_List_add(del_list, ptr, NULL);
			}
		}
		prev_hp = hp;
		DS_List_destroy(hp->retired, NULL);
		free(it);
	}
	free(prev_hp);
	DS_List_destroy(del_list, hazard_table->destructor);
	free(hazard_table);
	pthread_key_delete(tls);
}

static void scan(MU_Hazard_Pointer_t *hp){
	DS_List_t *private_list = DS_List_create(false);
	/*
		Loop through the hazard pointers owned by each thread. Add them to the private if they
		are non-NULL, hence in use.
	*/
	for(MU_Hazard_Pointer_t *tmp_hp = hazard_table->head; tmp_hp; tmp_hp = tmp_hp->next){
		for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
			if(tmp_hp->owned[i]){
				DS_List_add(private_list, tmp_hp->owned[i], NULL);
			}
		}
	}
<<<<<<< HEAD
	size_t arr_size = 0;
=======
	size_t arr_size = 0, tmp_arr_size = 0;
>>>>>>> development
	void **tmp_arr = DS_List_to_array(hp->retired, &arr_size);
	DS_List_clear(hp->retired, NULL);
	/*
		Since each thread has it's own list of retired pointers, we check to see
		if this thread's "retired" is no longer in the list of hazard pointers, hence
		no longer in use and can be freed.
	*/
<<<<<<< HEAD
	for(int i = 0; i < arr_size; i++){
		if(DS_List_contains(private_list, tmp_arr[i])){
			MU_LOG_TRACE(logger, "Added data to retirement from hazard table!");
			DS_List_add(hp->retired, tmp_arr[i], NULL);
		} else {
			hazard_table->destructor(tmp_arr[i]);
			MU_LOG_TRACE(logger, "Deleted data from hazard table!");
=======
	tmp_arr_size = arr_size;
	for(int i = 0; i < arr_size; i++){
		if(DS_List_contains(private_list, tmp_arr[i])){
			MU_LOG_TRACE(logger, "Added data to retirement from hazard table for HP #%zu!", hp->id);
			DS_List_add(hp->retired, tmp_arr[i], NULL);
		} else {
			hazard_table->destructor(tmp_arr[i]);
			MU_LOG_TRACE(logger, "Deleted data from hazard table #%zu! Size: %zu!", hp->id, --tmp_arr_size);
>>>>>>> development
		}
	}
	free(tmp_arr);
	DS_List_destroy(private_list, NULL);
}

static void help_scan(MU_Hazard_Pointer_t *hp){
	for(MU_Hazard_Pointer_t *tmp_hp = hazard_table->head; tmp_hp; tmp_hp = tmp_hp->next){
		// If we fail to mark the hazard pointer as active, then it's already in use.
		if(tmp_hp->in_use || !__sync_bool_compare_and_swap(&tmp_hp->in_use, false, true)) continue;
		void *data;
		while((data = DS_List_remove_at(tmp_hp->retired, 0, NULL))){
			DS_List_add(hp->retired, data, NULL);
			if(hp->retired->size >= max_hazard_pointers) scan(hp);
		}
		tmp_hp->in_use = false;
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

static void init_tls_hp(void){
<<<<<<< HEAD
=======
	static volatile int index = 0;
>>>>>>> development
	for(MU_Hazard_Pointer_t *tmp_hp = hazard_table->head; tmp_hp; tmp_hp = tmp_hp->next){
		if(tmp_hp->in_use || __sync_bool_compare_and_swap(&tmp_hp->in_use, false, true)) continue;
		pthread_setspecific(tls, tmp_hp);
		MU_LOG_TRACE(logger, "Was able to reclaim a previous hazard pointer!");
		return;
	}
	MU_Hazard_Pointer_t *hp = create();
	if(!hp){
		MU_LOG_ERROR(logger, "create_hp: 'Was unable to allocate a Hazard Pointer!");
		return;
	}
<<<<<<< HEAD
=======
	hp->id = index++;
>>>>>>> development
	MU_LOG_TRACE(logger, "Was unable to reclaim a previous hazard pointer, successfully created a new one!");
	MU_Hazard_Pointer_t *old_head;
	do {
		old_head = hazard_table->head;
		hp->next = old_head;
	} while(!__sync_bool_compare_and_swap(&hazard_table->head, old_head, hp));
	__sync_fetch_and_add(&hazard_table->size, MU_HAZARD_POINTERS_PER_THREAD);
	pthread_setspecific(tls, hp);
<<<<<<< HEAD
	MU_LOG_TRACE(logger, "Was successful in adding hazard pointer to hazard table!");
=======
	MU_LOG_TRACE(logger, "Was successful in adding hazard pointer #%zu to hazard table!", hp->id);
>>>>>>> development
}

bool MU_Hazard_Pointer_acquire(unsigned int index, void *data){
	MU_ARG_CHECK(logger, false, data, index <= MU_HAZARD_POINTERS_PER_THREAD);
	// Get the hazard pointer from thread-local storage if it is allocated.
	MU_Hazard_Pointer_t *hp = pthread_getspecific(tls);
	// If it hasn't been allocated, then we allocate it here.
	if(!hp){
		MU_LOG_TRACE(logger, "Hazard Pointer for this thread not allocated! Initializing...");
		init_tls_hp();
		hp = pthread_getspecific(tls);
		if(!hp){
			MU_LOG_ERROR(logger, "init_tls_hp: 'Was unable initialize thread-local storage!'");
			return false;
		}
	}
	hp->owned[index] = data;
<<<<<<< HEAD
	MU_LOG_TRACE(logger, "Acquired pointer to data at index %d!", index);
=======
	MU_LOG_TRACE(logger, "Hazard Pointer #%zu has acquired pointer to data at index %d!", hp->id, index);
>>>>>>> development
	return true;
}

bool MU_Hazard_Pointer_release_all(bool retire){
	// Get the hazard pointer from thread-local storage if it is allocated.
	MU_Hazard_Pointer_t *hp = pthread_getspecific(tls);
	// If it hasn't been allocated, then surely the current thread never acquired anything.
	if(!hp){
		MU_LOG_TRACE(logger, "Attempt to release all data when no thread-local storage was allocated!");
		return false;
	}
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		void *data = hp->owned[i];
		if(data){
			hp->owned[i] = NULL;
			if(retire){
				DS_List_add(hp->retired, data, NULL);
<<<<<<< HEAD
				MU_LOG_TRACE(logger, "Added data to retirement list!");
				if(hp->retired->size >= max_hazard_pointers){
					MU_LOG_TRACE(logger, "Retirement list filled, scanning...");
					scan(hp);
					MU_LOG_TRACE(logger, "Retirement list filled, help_scanning...");
=======
				MU_LOG_TRACE(logger, "Added data to retirement list for HP #%zu with size: %zu!", hp->id, hp->retired->size);
				if(hp->retired->size >= MU_HAZARD_POINTERS_PER_THREAD){
					MU_LOG_TRACE(logger, "Retirement list filled for HP #%zu, scanning...", hp->id);
					scan(hp);
					MU_LOG_TRACE(logger, "Retirement list filled for HP #%zu, help_scanning...", hp->id);
>>>>>>> development
					help_scan(hp);
				}
			}
		}
	}
	return true;
}

bool MU_Hazard_Pointer_release(void *data, bool retire){
	MU_ARG_CHECK(logger, false, data);
	// Get the hazard pointer from thread-local storage if it is allocated.
	MU_Hazard_Pointer_t *hp = pthread_getspecific(tls);
	// If it hasn't been allocated, then surely the current thread never acquired anything.
	if(!hp) return false;
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		if(hp->owned[i] == data){
			hp->owned[i] = NULL;
			if(retire){
				DS_List_add(hp->retired, data, NULL);
<<<<<<< HEAD
				MU_LOG_TRACE(logger, "Added data to retirement list, size: %zu!", hp->retired->size);
				if(hp->retired->size >= max_hazard_pointers){
					MU_LOG_TRACE(logger, "Retirement list filled, scanning...");
					scan(hp);
					MU_LOG_TRACE(logger, "Retirement list filled, help_scanning...");
=======
				MU_LOG_TRACE(logger, "Added data to retirement list for HP #%zu with size: %zu!", hp->id, hp->retired->size);
				if(hp->retired->size >= MU_HAZARD_POINTERS_PER_THREAD){
					MU_LOG_TRACE(logger, "Retirement list filled for HP #%zu, scanning...", hp->id);
					scan(hp);
					MU_LOG_TRACE(logger, "Retirement list filled for HP #%zu, help_scanning...", hp->id);
>>>>>>> development
					help_scan(hp);
				}
			}
		}
	}
	return true;
}

bool MU_Hazard_Pointer_register_destructor(void (*destructor)(void *)){
	MU_ARG_CHECK(logger, false, destructor);
	hazard_table->destructor = destructor;
	return true;
}
