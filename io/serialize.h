#ifndef C_UTILS_SERIALIZE_H
#define C_UTILS_SERIALIZE_H

#include "../misc/arg_count.h"

/*
	Allows the user to individually serialize a given primitive value.
	Note that this will automatically take the sizeof the value and
	also explicitly obtain the address of it, hence the caller should not
	pass a pointer.

	The buffer must be an initialized instance of buffer_t.
*/
#define C_UTILS_SERIALIZE_VALUE(buf, value) \
	do { \
		for(size_t i = 0; i < sizeof((value)); i++) \
      		buffer_append(buf, ((const unsigned char *)(&(value)))[i]  & 0xff, 1); \
	} while(0)

#define C_UTILS_CONCAT(a, b) a ## b

#define C_UTILS_SERIALIZE_ARGS(n) C_UTILS_CONCAT(C_UTILS_SERIALIZE_ARG_, n);

#define C_UTILS_SERIALIZE_OBJ(buf, obj, ...) C_UTILS_SERIALIZE_ARGS(C_UTILS_ARG_COUNT(__VA_ARGS__))(buf, obj, ...)

#define C_UTILS_SERIALIZE_ARG_16(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_15(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_15(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_14(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_14(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_13(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_13(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_12(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_12(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_11(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_11(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_10(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_10(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_9(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_9(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_8(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_8(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_7(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_7(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_6(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_6(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_5(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_5(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_4(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_4(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_3(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_3(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_2(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_2(buf, obj, arg, ...) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg)); \
  C_UTILS_SERIALIZE_ARG_1(buf, obj, __VA_ARGS__)

#define C_UTILS_SERIALIZE_ARG_1(buf, obj, arg) \
  C_UTILS_SERIALIZE_VALUE(buf, C_UTILS_CONCAT(obj., arg));


#endif /* C_UTILS_SERIALIZE_H */