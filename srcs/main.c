#include "tests/tests.h"
#include "stdio.h"

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    printf("tests_preinit()\n");
    tests_preinit();
    printf("tests_init()\n");
    tests_alloc_free_4MB();
    return 0;
}
