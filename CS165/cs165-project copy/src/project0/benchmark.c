#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "chained_hash_table.h"
#include "hash_table.h"
#include "robinhood_hash_table.h"

int main(void) {
  int num_tests = 50000000;

  ch_hashtable *c_ht = NULL;
  rh_hashtable *r_ht = NULL;

  assert(allocate(&c_ht, num_tests) == 0);
  assert(allocate(&r_ht, num_tests) == 0);

  int seed = 2;
  srand(seed);

  printf("Running stress test, insert %d entries...\n", num_tests);

  printf("Generating random numbers...\n");
  // Generate random numbers
  int *randoms = malloc(2 * num_tests * sizeof(int));
  for (int i = 0; i < 2 * num_tests; ++i) {
    randoms[i] = rand();
  }

  printf("(preallocated, chained):");

  struct timeval stop, start;
  gettimeofday(&start, NULL);

  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * i];
    int val = randoms[2 * i + 1];
    assert(put(c_ht, key, val) == 0);
  }

  gettimeofday(&stop, NULL);
  double secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
                (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);
  assert(deallocate(c_ht) == 0);

  printf("(preallocated, robinhood):");

  gettimeofday(&start, NULL);

  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * i];
    int val = randoms[2 * i + 1];
    assert(put(r_ht, key, val) == 0);
  }

  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  assert(deallocate(r_ht) == 0);

  assert(allocate(&c_ht, 1024) == 0);
  assert(allocate(&r_ht, 1024) == 0);

  printf("(dynamic, chained):");

  gettimeofday(&start, NULL);

  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * i];
    int val = randoms[2 * i + 1];
    assert(put(c_ht, key, val) == 0);
  }

  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  printf("(dynamic, robinhood):");

  gettimeofday(&start, NULL);

  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * i];
    int val = randoms[2 * i + 1];
    assert(put(r_ht, key, val) == 0);
  }

  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  printf("Running stress test, search %d entries (up to 5)...\n", num_tests);

  printf("(search, chained):");
  gettimeofday(&start, NULL);

  int num_values = 5;
  int results[num_values];
  int num_results;

  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * (randoms[i] % num_tests)];
    assert(get(c_ht, key, results, num_values, &num_results) == 0);
  }

  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  printf("(search, robinhood):");
  gettimeofday(&start, NULL);

  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * (randoms[i] % num_tests)];
    assert(get(r_ht, key, results, num_values, &num_results) == 0);
  }

  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  printf("Running stress test, delete %d entries...\n", num_tests);

  printf("(delete, chained):");
  gettimeofday(&start, NULL);
  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * (randoms[i] % num_tests)];
    assert(erase(c_ht, key) == 0);
  }
  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  printf("(delete, robinhood):");
  gettimeofday(&start, NULL);
  for (int i = 0; i < num_tests; i += 1) {
    int key = randoms[2 * (randoms[i] % num_tests)];
    assert(erase(r_ht, key) == 0);
  }
  gettimeofday(&stop, NULL);
  secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
         (double)(stop.tv_sec - start.tv_sec);
  printf(" %f\n", secs);

  assert(deallocate(r_ht) == 0);
  assert(deallocate(c_ht) == 0);
}
