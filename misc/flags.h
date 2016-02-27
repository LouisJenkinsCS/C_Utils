#ifndef C_UTILS_FLAGS_H
#define C_UTILS_FLAGS_H

#ifdef NO_C_UTILS_PREFIX
#define FLAG_GET(...) C_UTILS_FLAG_GET(__VA_ARGS__)
#define FLAG_SET(...) C_UTILS_FLAG_SET(__VA_ARGS__)
#define FLAG_CLEAR(...) C_UTILS_FLAG_CLEAR(__VA_ARGS__)
#define FLAG_TOGGLE(...) C_UTILS_FLAG_TOGGLE(__VA_ARGS__)
#endif

/**
* Simple macros for simple bitwise manipulations. The flag passed should be an unsigned or signed integer.
*/

/*
	Returns if flag is set in bitmask.
 */
#define C_UTILS_FLAG_GET(mask, flag) (mask & flag) 

/*
	Sets the flag inside of the bitmask.
 */
#define C_UTILS_FLAG_SET(mask, flag) (mask |= flag)

/*
	Clears the flag from the bitmask
 */
#define C_UTILS_FLAG_CLEAR(mask, flag) (mask &= (~flag))

/*
	Toggles the flag inside of the bitmask.
 */
#define C_UTILS_FLAG_TOGGLE(mask, flag) (mask ^= flag)


#endif /* endif C_UTILS_FLAGS_H */