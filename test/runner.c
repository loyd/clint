/*!
 * @brief Runner of tests.
 */

#include "clint.h"
#include "helper.h"


int main(void)
{
  pause_warnings();

  test_lexer();
  test_parser();

  return 0;
}
