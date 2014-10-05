/*!
 * @brief Runner of tests.
 */

#include "clint.h"
#include "helper.h"


int main(void)
{
  warn_pause();

  RUN(lexer);

  return 0;
}
