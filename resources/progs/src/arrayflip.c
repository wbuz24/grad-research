// Will Buziak 

// Generate a large array and randomly access indices

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIGARRAY 2000000000

int arr[BIGARRAY];

int main(int argc, char** argv) {
  int size, buf, i, count;

  if (argc != 2) { printf("Format:\n./arrtest <SIZE>\n"); exit(1); }
  
  if (sscanf(argv[1], "%d", &size) != 1) { printf("Invalid size, must be an integer\n"); exit(1); }

  memset(arr, '0', sizeof(arr));

  count = 0;
  for (i = 0; i < size; i++) {
    buf = (i * rand()) % (BIGARRAY - 1); // generate a random index
    if (buf < 0) buf = buf * -1; // check that it is positive
    if (arr[buf]) arr[buf] = 0; // flip the bit at the index
    else arr[buf] = 1;

    count++; // count iterations
  }

  printf("%d array accesses\n", count);

  return 0;
}
