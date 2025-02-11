#include "m61.hh"
#include <cstdio>
// realloc doesn't create leaks

int main() {
	void* ptr = malloc(100);
	realloc(ptr, 0);

  void* ptr2 = malloc(100);
  ptr2 = realloc(ptr2, 1000);
  free(ptr2);
  
	m61_print_leak_report();
}