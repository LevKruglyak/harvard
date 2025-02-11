#include "m61.hh"
#include <cstdio>
// realloc works like malloc when nullptr passed in

int main() {
	void* ptr = realloc(nullptr, 1);	
	free(ptr);
	m61_print_statistics();
}

//! alloc count: active          0   total          1   fail          0
//! alloc size:  active          0   total          1   fail          0