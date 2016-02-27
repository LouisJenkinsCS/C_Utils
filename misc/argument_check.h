#ifndef ARG_CHECK_H
#define ARG_CHECK_H

#include <stdarg.h>
#include <stdbool.h>

#include "io/logger.h"

#ifdef NO_C_UTILS_PREFIX
#define ARG_CHECK(...) C_UTILS_ARG_CHECK(__VA_ARGS__)
#endif


#define C_UTILS_ARG_MAX 8

/// Genius implementation someone else made to find the amount of variadic functions.
#define C_UTILS_ARG_COUNT(...) (sizeof((bool[]){__VA_ARGS__})/sizeof(bool))

#define C_UTILS_ARG_BOOL_EVAL(arg) c_utils_arg_evaluate_boolean(arg)

#define C_UTILS_ARG_TO_STRING(arg)("" #arg "")

/**
 * @param logger
 * @param retval
 * @param ...
 * @return 
 */
#define C_UTILS_ARG_CHECK(logger, retval, ...) do { \
	int num_args = C_UTILS_ARG_COUNT(__VA_ARGS__); \
	if(num_args < 0 || num_args > C_UTILS_ARG_MAX) break; \
	bool valid_args[C_UTILS_ARG_MAX]; \
	memset(&valid_args, true, sizeof(bool) * C_UTILS_ARG_MAX); \
	if(c_utils_arg_evaluate_arguments(num_args, valid_args, __VA_ARGS__)) break; \
	char *arg_str = calloc(1, 512); \
	sprintf(arg_str, "%s: Invalid Argument%s=> { ", __FUNCTION__, num_args > 1 ? "s" : ""); \
	switch(num_args){ \
		case 1: C_UTILS_ARG_CHECK_1(arg_str, valid_args, __VA_ARGS__); break; \
		case 2: C_UTILS_ARG_CHECK_2(arg_str, valid_args, __VA_ARGS__); break; \
		case 3: C_UTILS_ARG_CHECK_3(arg_str, valid_args, __VA_ARGS__); break; \
		case 4: C_UTILS_ARG_CHECK_4(arg_str, valid_args, __VA_ARGS__); break; \
		case 5: C_UTILS_ARG_CHECK_5(arg_str, valid_args, __VA_ARGS__); break; \
		case 6: C_UTILS_ARG_CHECK_6(arg_str, valid_args, __VA_ARGS__); break; \
		case 7: C_UTILS_ARG_CHECK_7(arg_str, valid_args, __VA_ARGS__); break; \
		case 8: C_UTILS_ARG_CHECK_8(arg_str, valid_args, __VA_ARGS__); break; \
		default: assert(0); \
	} \
	sprintf(arg_str, "%s!", arg_str); \
	C_UTILS_LOG_ERROR(logger, "%s }", arg_str); \
	free(arg_str); \
	return retval; \
} while(0)


#define C_UTILS_ARG_CHECK_8(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[7])); C_UTILS_ARG_CHECK_7(string_ptr, valid_args, __VA_ARGS__)

#define C_UTILS_ARG_CHECK_7(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[6])); C_UTILS_ARG_CHECK_6(string_ptr, valid_args,  __VA_ARGS__)

#define C_UTILS_ARG_CHECK_6(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[5])); C_UTILS_ARG_CHECK_5(string_ptr, valid_args,  __VA_ARGS__)

#define C_UTILS_ARG_CHECK_5(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[4])); C_UTILS_ARG_CHECK_4(string_ptr, valid_args,  __VA_ARGS__)

#define C_UTILS_ARG_CHECK_4(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[3])); C_UTILS_ARG_CHECK_3(string_ptr, valid_args,  __VA_ARGS__)

#define C_UTILS_ARG_CHECK_3(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[2])); C_UTILS_ARG_CHECK_2(string_ptr, valid_args,  __VA_ARGS__)

#define C_UTILS_ARG_CHECK_2(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[1])); C_UTILS_ARG_CHECK_1(string_ptr, valid_args,  __VA_ARGS__)

#define C_UTILS_ARG_CHECK_1(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s", string_ptr,  C_UTILS_ARG_TO_STRING(arg), c_utils_arg_evaluate_boolean(valid_args[0]));

bool c_utils_arg_evaluate_arguments(int num_args, bool *arr, ...){
	int i = num_args - 1;
	
	va_list list;
	va_start(list, arr);
	
	bool result = true;
	for(; i >= 0; i--){
		bool is_valid = va_arg(list, int);
		arr[i] = is_valid;
		if(!is_valid)
			result = false;
	}

	return result;
}

char *c_utils_arg_evaluate_boolean(bool arg){
	return arg ? "TRUE" : "FALSE";
}

#endif /* endif MU_ARG_CHECK_H */