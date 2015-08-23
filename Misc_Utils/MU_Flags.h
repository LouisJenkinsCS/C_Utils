#ifndef MU_FLAGS_H
#define MU_FLAGS_H

/**
* Simple macros for simple bitwise manipulations. The flag passed should be an unsigned or signed integer.
*/

/**
 * @param mask
 * @param flag
 * @return
 */
#define MU_FLAG_GET(mask, flag) (mask & flag) 

/**
 * @param mask
 * @param flag
 * @return
 */
#define MU_FLAG_SET(mask, flag) (mask |= flag)

/**
 * @param mask
 * @param flag
 * @return
 */
#define MU_FLAG_CLEAR(mask, flag) (mask &= (~flag))

/**
 * @param mask
 * @param flag
 * @return
 */
#define MU_FLAG_TOGGLE(mask, flag) (mask ^= flag)


#endif /* endif MU_FLAGS_H */