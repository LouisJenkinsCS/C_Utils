#include <stdlib.h>

#include "hazard.h"
#include "../io/logger.h"
#include "../data_structures/list.h"
#include "../misc/alloc_check.h"
#include "../misc/argument_check.h"

struct c_utils_hazard {
	volatile bool in_use;
	size_t id;
	struct c_utils_list *retired;
	void *owned[C_UTILS_HAZARD_PER_THREAD];
	void (*destructor)(void *);
	struct c_utils_hazard *next;
};

struct c_utils_hazard_list {
	struct c_utils_hazard *head;
	volatile size_t size;
	void (*destructor)(void *);
};

static struct c_utils_hazard_list *hazard_table = NULL;

static pthread_key_t tls;

static struct c_utils_logger *logger = NULL;

static const int max_hazard_pointers = C_UTILS_HAZARD_THREADS * C_UTILS_HAZARD_PER_THREAD;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./memory/logs/hazard.log", "w", C_UTILS_LOG_LEVEL_INFO);

__attribute__((constructor)) static void init_hazard_table(void) {
	C_UTILS_ON_BAD_CALLOC(hazard_table, logger, sizeof(*hazard_table))
		return;

	hazard_table->destructor = free;
	hazard_table->size = 0;
}

__attribute__((constructor)) static void init_tls_key(void) {
	pthread_key_create(&tls, NULL);
}

/*
	TODO: Major, please optimize this thing, I can't even tell it's complexity. I think
	O(N^3)? Regardless, it's awfully inefficient, and a hash table would at least make it O(N).
*/
__attribute__((destructor)) static void destroy_hazard_table(void) {
	struct c_utils_hazard *prev_hp = NULL;
	struct c_utils_list *del_list = c_utils_list_create(false);

	for (struct c_utils_hazard *hp = hazard_table->head; hp; hp = hp->next) {
		free(prev_hp);
		struct c_utils_iterator *it = c_utils_list_iterator(hp->retired);

		for (int i = 0; i < C_UTILS_HAZARD_PER_THREAD; i++)
			if (hp->owned[i] && !c_utils_list_contains(del_list, hp->owned[i]))
				c_utils_list_add(del_list, hp->owned[i], NULL);

		void *ptr;
		while ((ptr = c_utils_iterator_next(it)))
			if (!c_utils_list_contains(del_list, ptr))
				c_utils_list_add(del_list, ptr, NULL);
		
		prev_hp = hp;
		c_utils_list_destroy(hp->retired, NULL);
		free(it);
	}
	free(prev_hp);
	c_utils_list_destroy(del_list, hazard_table->destructor);
	free(hazard_table);
	pthread_key_delete(tls);
}

static void scan(struct c_utils_hazard *hp) {
	struct c_utils_list *private_list = c_utils_list_create(false);
	/*
		Loop through the hazard pointers owned by each thread. Add them to the private if they
		are non-NULL, hence in use.
	*/
	for (struct c_utils_hazard *tmp_hp = hazard_table->head; tmp_hp; tmp_hp = tmp_hp->next)
		for (int i = 0; i < C_UTILS_HAZARD_PER_THREAD; i++)
			if (tmp_hp->owned[i])
				c_utils_list_add(private_list, tmp_hp->owned[i], NULL);
	
	size_t arr_size = 0, tmp_arr_size = 0;
	void **tmp_arr = c_utils_list_as_array(hp->retired, &arr_size);
	c_utils_list_clear(hp->retired, NULL);
	
	/*
		Since each thread has it's own list of retired pointers, we check to see
		if this thread's "retired" is no longer in the list of hazard pointers, hence
		no longer in use and can be freed.
	*/
	tmp_arr_size = arr_size;
	for (int i = 0; i < arr_size; i++)
		if (c_utils_list_contains(private_list, tmp_arr[i])) {
			C_UTILS_LOG_TRACE(logger, "Added data to retirement from hazard table for HP #%zu!", hp->id);
			c_utils_list_add(hp->retired, tmp_arr[i], NULL);
		} else {
			hazard_table->destructor(tmp_arr[i]);
			C_UTILS_LOG_TRACE(logger, "Deleted data from hazard table #%zu! Size: %zu!", hp->id, --tmp_arr_size);
		}

	free(tmp_arr);
	c_utils_list_destroy(private_list, NULL);
}

static void help_scan(struct c_utils_hazard *hp) {
	for (struct c_utils_hazard *tmp_hp = hazard_table->head; tmp_hp; tmp_hp = tmp_hp->next) {
		// If we fail to mark the hazard pointer as active, then it's already in use.
		if (tmp_hp->in_use || !__sync_bool_compare_and_swap(&tmp_hp->in_use, false, true)) 
			continue;

		void *data;
		while ((data = c_utils_list_remove_at(tmp_hp->retired, 0, NULL))) {
			c_utils_list_add(hp->retired, data, NULL);
			
			if (c_utils_list_size(hp->retired) >= max_hazard_pointers) 
				scan(hp);
		}

		tmp_hp->in_use = false;
	}
}

static struct c_utils_hazard *create() {
	struct c_utils_hazard *hp;
	C_UTILS_ON_BAD_CALLOC(hp, logger, sizeof(*hp))
		goto err;
	hp->in_use = true;
	
	hp->retired = c_utils_list_create(false);
	if (!hp->retired) {
		C_UTILS_LOG_ERROR(logger, "c_utils_list_create: '%s'", strerror(errno));
		goto err_list;
	}
	
	return hp;

	err_list:
		free(hp);
	err:
		return NULL;
}

static void init_tls_hp(void) {
	static volatile int index = 0;
	for (struct c_utils_hazard *tmp_hp = hazard_table->head; tmp_hp; tmp_hp = tmp_hp->next) {
		if (tmp_hp->in_use || __sync_bool_compare_and_swap(&tmp_hp->in_use, false, true)) 
			continue;
		
		pthread_setspecific(tls, tmp_hp);
		C_UTILS_LOG_TRACE(logger, "Was able to reclaim a previous hazard pointer!");
		
		return;
	}
	
	struct c_utils_hazard *hp = create();
	if (!hp) {
		C_UTILS_LOG_ERROR(logger, "create_hp: 'Was unable to allocate a Hazard Pointer!");
		return;
	}
	hp->id = index++;
	C_UTILS_LOG_TRACE(logger, "Was unable to reclaim a previous hazard pointer, successfully created a new one!");
	
	struct c_utils_hazard *old_head;
	do {
		old_head = hazard_table->head;
		hp->next = old_head;
	} while (!__sync_bool_compare_and_swap(&hazard_table->head, old_head, hp));
	__sync_fetch_and_add(&hazard_table->size, C_UTILS_HAZARD_PER_THREAD);
	
	pthread_setspecific(tls, hp);
	C_UTILS_LOG_TRACE(logger, "Was successful in adding hazard pointer #%zu to hazard table!", hp->id);
}

bool c_utils_hazard_acquire(unsigned int index, void *data) {
	C_UTILS_ARG_CHECK(logger, false, data, index <= C_UTILS_HAZARD_PER_THREAD);
	
	// Get the hazard pointer from thread-local storage if it is allocated.
	struct c_utils_hazard *hp = pthread_getspecific(tls);
	// If it hasn't been allocated, then we allocate it here.
	if (!hp) {
		C_UTILS_LOG_TRACE(logger, "Hazard Pointer for this thread not allocated! Initializing...");
		init_tls_hp();
		
		hp = pthread_getspecific(tls);
		if (!hp) {
			C_UTILS_LOG_ERROR(logger, "init_tls_hp: 'Was unable initialize thread-local storage!'");
			return false;
		}
	}
	hp->owned[index] = data;
	
	C_UTILS_LOG_TRACE(logger, "Hazard Pointer #%zu has acquired pointer to data at index %d!", hp->id, index);
	return true;
}

bool c_utils_hazard_release_all(bool retire) {
	// Get the hazard pointer from thread-local storage if it is allocated.
	struct c_utils_hazard *hp = pthread_getspecific(tls);
	// If it hasn't been allocated, then surely the current thread never acquired anything.
	if (!hp) {
		C_UTILS_LOG_TRACE(logger, "Attempt to release all data when no thread-local storage was allocated!");
		return false;
	}
	
	for (int i = 0; i < C_UTILS_HAZARD_PER_THREAD; i++) {
		void *data = hp->owned[i];
		if (data) {
			hp->owned[i] = NULL;
			if (retire) {
				c_utils_list_add(hp->retired, data, NULL);
				C_UTILS_LOG_TRACE(logger, "Added data to retirement list for HP #%zu with size: %zu!", hp->id, c_utils_list_size(hp->retired));
				
				if (c_utils_list_size(hp->retired) >= C_UTILS_HAZARD_PER_THREAD) {
					C_UTILS_LOG_TRACE(logger, "Retirement list filled for HP #%zu, scanning...", hp->id);
					scan(hp);
					
					C_UTILS_LOG_TRACE(logger, "Retirement list filled for HP #%zu, help_scanning...", hp->id);
					help_scan(hp);
				}
			}
		}
	}

	return true;
}

bool c_utils_hazard_release(void *data, bool retire) {
	C_UTILS_ARG_CHECK(logger, false, data);
	
	// Get the hazard pointer from thread-local storage if it is allocated.
	struct c_utils_hazard *hp = pthread_getspecific(tls);
	// If it hasn't been allocated, then surely the current thread never acquired anything.
	if (!hp)
		return false;
	
	for (int i = 0; i < C_UTILS_HAZARD_PER_THREAD; i++) {
		if (hp->owned[i] == data) {
			hp->owned[i] = NULL;
			if (retire) {
				c_utils_list_add(hp->retired, data, NULL);
				C_UTILS_LOG_TRACE(logger, "Added data to retirement list for HP #%zu with size: %zu!", hp->id, c_utils_list_size(hp->retired));
				if (c_utils_list_size(hp->retired) >= C_UTILS_HAZARD_PER_THREAD) {
					C_UTILS_LOG_TRACE(logger, "Retirement list filled for HP #%zu, scanning...", hp->id);
					scan(hp);
					
					C_UTILS_LOG_TRACE(logger, "Retirement list filled for HP #%zu, help_scanning...", hp->id);
					help_scan(hp);
				}
			}
		}
	}

	return true;
}

bool c_utils_hazard_register_destructor(void (*destructor)(void *)) {
	C_UTILS_ARG_CHECK(logger, false, destructor);
	
	hazard_table->destructor = destructor;
	return true;
}
