#include "test.h"

int main() {
    if (run_tests()) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("FAILED\n");
    return 1;
}
