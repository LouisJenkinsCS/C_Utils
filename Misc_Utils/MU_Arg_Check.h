#ifndef MU_ARG_CHECK_H
#define MU_ARG_CHECK_H

#include <MU_Logger.h>
#include <stdarg.h>
#include <stdbool.h>

#define MU_ARG_MAX 8

/// Genius implementation someone else made to find the amount of variadic functions.
#define MU_ARG_COUNT(...) (sizeof((bool[]){__VA_ARGS__})/sizeof(bool))

#define MU_ARG_BOOL_EVAL(arg) MU_Arg_evaluate_boolean(arg)

#define MU_ARG_TO_STRING(arg)("" #arg "")

/**
 * @param logger
 * @param retval
 * @param ...
 * @return 
 */
#define MU_ARG_CHECK(logger, retval, ...) do { \
	int num_args = MU_ARG_COUNT(__VA_ARGS__); \
	if(num_args < 0 || num_args > MU_ARG_MAX) break; \
	bool valid_args[MU_ARG_MAX]; \
	memset(&valid_args, true, sizeof(bool) * MU_ARG_MAX); \
	if(MU_Arg_evaluate_arguments(num_args, valid_args, __VA_ARGS__)) break; \
	char *arg_str = calloc(1, 512); \
	sprintf(arg_str, "%s: Invalid Argument%s=> { ", __FUNCTION__, num_args > 1 ? "s" : ""); \
	switch(num_args){ \
		case 1: MU_ARG_CHECK_1(arg_str, valid_args, __VA_ARGS__); break; \
		case 2: MU_ARG_CHECK_2(arg_str, valid_args, __VA_ARGS__); break; \
		case 3: MU_ARG_CHECK_3(arg_str, valid_args, __VA_ARGS__); break; \
		case 4: MU_ARG_CHECK_4(arg_str, valid_args, __VA_ARGS__); break; \
		case 5: MU_ARG_CHECK_5(arg_str, valid_args, __VA_ARGS__); break; \
		case 6: MU_ARG_CHECK_6(arg_str, valid_args, __VA_ARGS__); break; \
		case 7: MU_ARG_CHECK_7(arg_str, valid_args, __VA_ARGS__); break; \
		case 8: MU_ARG_CHECK_8(arg_str, valid_args, __VA_ARGS__); break; \
		default: assert(0); \
	} \
	sprintf(arg_str, "%s!", arg_str); \
	MU_LOG_ERROR(logger, "%s }", arg_str); \
	free(arg_str); \
	return retval; \
} while(0)


#define MU_ARG_CHECK_8(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[7])); MU_ARG_CHECK_7(string_ptr, valid_args, __VA_ARGS__)

#define MU_ARG_CHECK_7(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[6])); MU_ARG_CHECK_6(string_ptr, valid_args,  __VA_ARGS__)

#define MU_ARG_CHECK_6(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[5])); MU_ARG_CHECK_5(string_ptr, valid_args,  __VA_ARGS__)

#define MU_ARG_CHECK_5(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[4])); MU_ARG_CHECK_4(string_ptr, valid_args,  __VA_ARGS__)

#define MU_ARG_CHECK_4(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[3])); MU_ARG_CHECK_3(string_ptr, valid_args,  __VA_ARGS__)

#define MU_ARG_CHECK_3(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[2])); MU_ARG_CHECK_2(string_ptr, valid_args,  __VA_ARGS__)

#define MU_ARG_CHECK_2(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s;", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[1])); MU_ARG_CHECK_1(string_ptr, valid_args,  __VA_ARGS__)

#define MU_ARG_CHECK_1(string_ptr, valid_args, arg, ...) sprintf(string_ptr, "%s %s: %s", string_ptr,  MU_ARG_TO_STRING(arg), MU_Arg_evaluate_boolean(valid_args[0]));

bool MU_Arg_evaluate_arguments(int num_args, bool *arr, ...);

char *MU_Arg_evaluate_boolean(bool arg);

#endif /* endif MU_ARG_CHECK_H */