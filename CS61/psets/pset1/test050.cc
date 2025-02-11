#include "m61.hh"
#include "stdio.h"
// reallocation should throw error for invalid pointer

int main() {
  void* ptr = malloc(10);
  //printf("Will realloc %p\n", (char*) ptr + 10);
  ptr = realloc((char*) ptr + 10, 20);
  free(ptr);
}

//! MEMORY BUG???: invalid realloc of pointer ???, not allocated
//! ???