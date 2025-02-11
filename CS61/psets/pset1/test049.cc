#include "m61.hh"
// reallocation should resize block

int main() {
  char* ptr = (char*) malloc(10);
  ptr = (char*) realloc(ptr, 20);
  ptr[11] = 0;
  ptr[15] = 0;
  // There shouldn't be a wild write error here
  free(ptr);
  printf("no wild write\n");

  ptr = (char*) malloc(10);
  ptr = (char*) realloc(ptr, 20);
  ptr[20] = 0;
  // There should be a wild write error here
  free(ptr);
}

//! ???
//! MEMORY BUG???: detected wild write during free of pointer ???
//! ???