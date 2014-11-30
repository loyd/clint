/*!
 * @brief Runner of tests.
 */

#include "clint.h"
#include "helper.h"


int main(void)
{
    set_log_mode(LOG_NOTHING);

    test_lexer();
    test_parser();

    return 0;
}
