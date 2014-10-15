/*!
 * @brief Runner of tests.
 */

#include "clint.h"
#include "helper.h"


int main(void)
{
  pause_warnings();

  RUN(lexer);

  return 0;
}
