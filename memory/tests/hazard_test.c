#define NO_C_UTILS_PREFIX
#include "../hazard.h"
#include <stdlib.h>

struct hp_test {
	bool val;
};

/*
	Checks for memory leakage.
*/
int main(void) {
	for (int i = 0; i < 100; i++)
		hazard_acquire(0, malloc(sizeof(struct hp_test)));

	hazard_release_all(true);
	return 0;
}