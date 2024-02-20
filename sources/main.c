#include "tests/tests.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    printf("tests_preinit()\n");
    tests_preinit();
    printf("tests_small_sizes_predetermined()\n");
    tests_small_sizes_predetermined();
    printf("tests_small_sizes_predetermined2()\n");
    tests_small_sizes_predetermined2();
    printf("tests_random()\n");
    tests_random();
    printf("OK!\n");
    return 0;
}
