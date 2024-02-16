#include "tests/tests.h"
#include "stdio.h"

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    printf("tests_preinit()\n");
    tests_preinit();
    printf("tests_small_sizes()\n");
    tests_small_sizes();
    return 0;
}
