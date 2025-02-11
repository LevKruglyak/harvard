#include <stdio.h>
#include <stdlib.h>
#include "include/vector.h"


int main() {
  vec_str *vec = new_vec_str(10);
  push_vec_str(vec, "Hello, World! 1\n");
  push_vec_str(vec, "Hello, World! 2\n");
  push_vec_str(vec, "Hello, World! 3\n");
  push_vec_str(vec, "Hello, World! 4\n");

  printf("%s\n", get_vec_str(vec, 2));

  delete_vec_str(vec);
}
