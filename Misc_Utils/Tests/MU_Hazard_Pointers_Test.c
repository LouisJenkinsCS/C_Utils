#include <MU_Logger.h>
#include <MU_Hazard_Pointers.h>

struct hp_test {
	bool val;
};

void segfault(void *data){
	free(data);
	free(data);
}

int main(void){
	MU_Hazard_Pointer_register_destructor(segfault);
	MU_Hazard_Pointer_t *hp = MU_Hazard_Pointer_acquire();
	for(int i = 0; i < MU_HAZARD_POINTERS_PER_THREAD; i++){
		hp->owned[i] = malloc(sizeof(struct hp_test));
	}
	MU_Hazard_Pointer_release(hp);
	return 0;
}