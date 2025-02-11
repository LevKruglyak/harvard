#pragma once

#include <unistd.h>

static long number_of_processors() {
  long min_proc_number = 4;
  long num_proc = sysconf(_SC_NPROCESSORS_ONLN);
  if (num_proc < min_proc_number) {
    return min_proc_number;
  }
  return num_proc;
}
