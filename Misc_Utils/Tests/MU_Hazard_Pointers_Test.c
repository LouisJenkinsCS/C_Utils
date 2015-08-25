#include <MU_Logger.h>
#include <MU_Hazard_Pointers.h>

struct hp_test {
	bool val;
};

int main(void){
	MU_Hazard_Pointer_List_t *list = MU_Hazard_Pointer_init(free);
	MU_Hazard_Pointer_t *hp = MU_Hazard_Pointer_get(list);
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		MU_Hazard_Pointer_acquire(list, hp, malloc(sizeof(struct hp_test)));
	}
	MU_Hazard_Pointer_release_all(list, hp);
	return 0;
}