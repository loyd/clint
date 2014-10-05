/*!
 * @brief Simple macros for testing.
 */

#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdio.h>


#define RUN(name)                                                             \
    extern void group_ ## name(void);                                         \
    printf("> Group " #name ": \n");                                          \
    group_ ## name();                                                         \

#define GROUP(name)                                                           \
    void group_ ## name(void)                                                 \

#define TEST(name)                                                            \
    printf(">>> Testing " #name "...\n");                                     \
    if (1)

#endif  // __HELPER_H__
