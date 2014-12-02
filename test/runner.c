/*!
 * @brief Runner of tests.
 */

#include "clint.h"
#include "helper.h"


int main(void)
{
    g_log_mode |= LOG_SILENCE;

    test_lexer();
    test_parser();

    return 0;
}
