#ifndef C_UTILS_ARG_COUNT_H
#define C_UTILS_ARG_COUNT_H

#define C_UTILS_ARG_COUNT(...) C_UTILS_ARG_COUNT_(__VA_ARGS__, C_UTILS_ARG_NUMS())
#define C_UTILS_ARG_COUNT_(...) C_UTILS_ARG_GET(__VA_ARGS__)
#define C_UTILS_ARG_GET( _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, _11, _12, _13, _14, _15, _16, N, ...) N
#define C_UTILS_ARG_NUMS() 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#endif /* C_UTILS_ARG_COUNT_H */