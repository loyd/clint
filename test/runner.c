/*!
 * @brief Runner of tests.
 */

#include "clint.h"
#include "helper.h"

extern void test_lexer(void);
extern void test_parser(void);

int main(void)
{
  pause_warnings();

  test_lexer();
  test_parser();

  return 0;
}
