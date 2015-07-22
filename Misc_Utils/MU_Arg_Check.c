#include <MU_Arg_Check.h>

void MU_Arg_append_to_string(bool cond, char *cond_str, char **string_ptr){
	char *tmp_str;
	asprintf(&tmp_str, ";%s: %s", cond_str, MU_ARG_EVAL(cond));
	char *old_string_ptr = *string_ptr;
	asprintf(string_ptr, "%s%s", *string_ptr, tmp_str);
	free(old_string_ptr);
}

bool MU_Arg_evaluate_arguments(int num_args, ...){
	int i = 0;
	va_list list;
	va_start(list, num_args);
	bool result = true;
	for(; i < num_args; i++){
		if(!va_arg(list, int)){
			result = false;
			break;
		}
	}
	return result;
}

bool MU_Arg_to_bool(void *arg){
	return arg;
}