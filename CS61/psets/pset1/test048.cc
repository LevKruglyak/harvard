#include "m61.hh"
#include <cstdio>
#include <cassert>
// realloc throws error if unallocated pointer passed in 

int main() {
    void* ptr = malloc(100);
    fprintf(stderr, "bad pointer %p\n", (char*) ptr + 10);
    ptr = realloc((char*) ptr + 10, 100);
    free(ptr);
}

//! bad pointer ??{0x\w+}=ptr??
//! MEMORY BUG: ???: invalid realloc of pointer ??ptr??, not allocated