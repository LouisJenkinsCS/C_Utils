#ifndef MU_FLAGS_H
#define MU_FLAGS_H

/**
* Simple macros for simple bitwise manipulations. The flag passed should be an unsigned or signed integer.
*/

/// Determines if the flag is in the bitmask.
#define MU_FLAG_GET(mask, flag) (mask & flag) 

/// Sets the flag in the bitmask.
#define MU_FLAG_SET(mask, flag) (mask |= flag)

/// Clears the flag from the bitmask.
#define MU_FLAG_CLEAR(mask, flag) (mask &= (~flag))

/// Toggles the flag in the bitmask.
#define MU_FLAG_TOGGLE(mask, flag) (mask ^= flag)


#endif /* endif MU_FLAGS_H */