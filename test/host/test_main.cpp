#include "host_test.h"

int g_failures = 0;
int g_checks = 0;

int main()
{
    test_qbit_chain();
    test_qbit_address();
    test_explorer_parse();
    printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
