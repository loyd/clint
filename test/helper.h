/*!
 * @brief Simple macros for testing.
 */

#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdio.h>


#define group(name) printf("\n> Group %s:\n", name);
#define test(name) printf(">>> Testing %s...\n", name);

#endif  // __HELPER_H__
