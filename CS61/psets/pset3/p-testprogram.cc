#include "u-lib.hh"

void process_main() {
    sys_fork();
    sys_exit();

    while (true) {
        sys_yield();
    }
}
