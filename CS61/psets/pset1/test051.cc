#include "m61.hh"
#include "stdlib.h"
#include "string.h"
// make sure data is copied over for realloc

int main() {
  char* data = (char*) malloc(100);
  strcpy(data, "hello, world!");

  data = (char*) realloc(data, 150);
  assert(strcmp(data, "hello, world!") == 0);

  // Also make sure this doesn't cause a wild write
  data = (char*) realloc(data, 5);
  free(data);
}