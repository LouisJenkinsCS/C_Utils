#include <MU_Arg_Check.h>

bool MU_Arg_evaluate_arguments(int num_args, bool *arr, ...){
	int i = 0;
	va_list list;
	va_start(list, arr);
	bool result = true;
	for(; i < num_args; i++){
		bool is_valid = va_arg(list, int);
		arr[i] = is_valid;
		if(!is_valid){
			result = false;
		}
	}
	return result;
}

char *MU_Arg_evaluate_boolean(bool arg){
	return arg ? "TRUE" : "FALSE";
}