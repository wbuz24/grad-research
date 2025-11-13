// Will Buziak 

// Generate a large array and randomly access indices

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gem5/m5ops.h>

int main(int argc, char** argv) {
  unsigned long int size, iters, buf, i;
  int *arr;

  if (argc != 3) { printf("Format:\n./arrtest <ITERATIONS> <SIZE>\n"); exit(1); }
  
  if (sscanf(argv[1], "%ld", &iters) != 1) { printf("Invalid size, must be an integer\n"); exit(1); }
  if (sscanf(argv[2], "%ld", &size) != 1) { printf("Invalid number of iterations, must be an integer\n"); exit(1); }

  arr = (int *) malloc(sizeof(int) * size);
  if (arr == NULL) { printf("Error: malloc\n"); exit(1); }
  memset(arr, 0, size);

  #ifdef GEM5
    m5op_addr = 0xFFFF0000;
    map_m5_mem();
    m5_work_begin_addr(0,0);
  #endif

  for (i = 0; i < iters; i++) {
    buf = rand() % size; // generate a random index
    printf("Access %ld at index: %ld\n", i, buf);
    arr[buf]++;
  }

  printf("%ld array accesses on an array with %ld bytes\n", iters, size);

  #ifdef GEM5
    m5_work_end_addr(0,0);
    unmap_m5_mem();
  #endif

  return 0;
}
