#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>

#define RUN(name)                                                             \
  extern void test_ ## name(void);                                            \
  printf("Testing " #name "... ");                                            \
  test_ ## name();                                                            \
  printf("passed.")

#define TEST(name)                                                            \
  void test_ ## name(void)                                                    \

#endif
