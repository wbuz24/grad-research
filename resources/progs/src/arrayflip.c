// Will Buziak 

// Generate a large array and randomly access indices

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gem5/m5ops.h>

int main(int argc, char** argv) {
  unsigned long int size, iters, buf, i;
  int *arr;

  if (argc != 3) { printf("Format:\n./arrtest <SIZE> <ITERATIONS>\n"); exit(1); }
  
  if (sscanf(argv[1], "%ld", &size) != 1) { printf("Invalid size, must be an integer\n"); exit(1); }
  if (sscanf(argv[1], "%ld", &iters) != 1) { printf("Invalid number of iterations, must be an integer\n"); exit(1); }

  arr = (int *) malloc(sizeof(int) * size);
  memset(arr, 0, size);

  #ifdef GEM5
    // gem5 exit for checkpoint
    m5.switchCpus();
  #endif // GEM5

  printf("CPUs switched?\n");

  for (i = 0; i < iters; i++) {
    buf = rand() % size; // generate a random index
    arr[buf]++;
  }

  printf("%ld array accesses\n", iters);

  #ifdef GEM5
    m5_exit(0);
  #endif // GEM5

  return 0;
}
