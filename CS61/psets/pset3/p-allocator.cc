#include "u-lib.hh"
#ifndef ALLOC_SLOWDOWN
#define ALLOC_SLOWDOWN 100
#endif

extern uint8_t end[];

// These global variables go on the data page.
uint8_t* heap_top;
uint8_t* stack_bottom;

void process_main() {
    pid_t p = sys_getpid();
    srand(p);

    // The heap starts on the page right after the 'end' symbol,
    // whose address is the first address not allocated to process code
    // or data.
    heap_top = (uint8_t*)round_up((uintptr_t)end, PAGESIZE);

    // The bottom of the stack is the first address on the current
    // stack page (this process never needs more than one stack page).
    stack_bottom = (uint8_t*)round_down((uintptr_t)rdrsp() - 1, PAGESIZE);

    // Allocate heap pages until (1) hit the stack (out of address space)
    // or (2) allocation fails (out of physical memory).
    while (heap_top != stack_bottom) {
        if (rand(0, ALLOC_SLOWDOWN - 1) < p) {
            if (sys_page_alloc(heap_top) < 0) {
                break;
            }
            // check that the page starts out all zero
            for (unsigned long* l = (unsigned long*)heap_top;
                 l != (unsigned long*)(heap_top + PAGESIZE);
                 ++l) {
                assert(*l == 0);
            }
            // check we can write to new page
            *heap_top = p;
            // check we can write to console
            console[CPOS(24, 79)] = p;
            // update `heap_top`
            heap_top += PAGESIZE;
        }
        sys_yield();
    }

    // After running out of memory, do nothing forever
    while (true) {
        sys_yield();
    }
}
