#ifndef MU_FLAGS_H
#define MU_FLAGS_H

#ifdef C_UTILS_USE_POSIX_STD
#define FLAG_GET(...) MU_FLAG_GET(__VA_ARGS__)
#define FLAG_SET(...) MU_FLAG_SET(__VA_ARGS__)
#define FLAG_CLEAR(...) MU_FLAG_CLEAR(__VA_ARGS__)
#define FLAG_TOGGLE(...) MU_FLAG_TOGGLE(__VA_ARGS__)
#endif

/**
* Simple macros for simple bitwise manipulations. The flag passed should be an unsigned or signed integer.
*/

/*
	Returns if flag is set in bitmask.
 */
#define MU_FLAG_GET(mask, flag) (mask & flag) 

/*
	Sets the flag inside of the bitmask.
 */
#define MU_FLAG_SET(mask, flag) (mask |= flag)

/*
	Clears the flag from the bitmask
 */
#define MU_FLAG_CLEAR(mask, flag) (mask &= (~flag))

/*
	Toggles the flag inside of the bitmask.
 */
#define MU_FLAG_TOGGLE(mask, flag) (mask ^= flag)


#endif /* endif MU_FLAGS_H */