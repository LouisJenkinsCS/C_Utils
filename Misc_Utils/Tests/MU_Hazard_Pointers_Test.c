#include <MU_Logger.h>
#include <MU_Hazard_Pointers.h>

struct hp_test {
	bool val;
};

int main(void){
	for(int i = 0; i < 100; i++){
		MU_Hazard_Pointer_acquire(malloc(sizeof(struct hp_test)));
	}
	MU_Hazard_Pointer_release_all(true);
	return 0;
}