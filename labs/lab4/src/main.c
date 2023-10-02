#include "test.h"

int main() {
    if (run_tests()) {
        printf("ALL TESTS PASSED");
        return 0;
    }
    printf("FAILED");
    return 1;
}
