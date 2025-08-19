// Will Buziak 

// Generate a large array and randomly access indices

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  int size, iter, buf, i, count, *arr;

  if (argc != 2) { printf("Format:\n./arrtest <SIZE>\n"); exit(1); }
  
  if (sscanf(argv[1], "%d", &size) != 1) { printf("Invalid size, must be an integer\n"); exit(1); }

  iter = size - 1;

  arr = (int *) malloc(sizeof(int) * 2000000000); // create an array of ints the size of 2GB 
  if (arr == NULL) { fprintf(stderr, "malloc(%d) failed.\n", size); exit(1); }

  memset(arr, '0', 2000000000);

  count = 0;
  for (i = 0; i < iter; i++) {
    buf = (i * rand()) % (2000000000 - 1); // generate a random index
    if (buf < 0) buf = buf * -1; // check that it is positive
    if (arr[buf]) arr[buf] = 0; // flip the bit at the index
    else arr[buf] = 1;

    count++; // count iterations
  }

  printf("%d array accesses\n", count);

  free(arr);
  arr = NULL;

  return 0;
}
