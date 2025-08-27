#include <gem5/m5ops.h>

#include <cassert>
#include <cstring>
#include <iostream>

using namespace std;

int
main(int argc, char **argv)
{
    assert(argc == 3);
    uint64_t arr_size = stoi(argv[1]);
    uint64_t num_ops = stoi(argv[2]);

    cout << "arr size: " << arr_size << endl;
    cout << "num ops : " << num_ops  << endl;

    // initialize array
    char *arr = (char *) malloc(sizeof(char) * arr_size);
    memset(arr, 1, arr_size); // initialize all array values to 1

    cout << "About to checkpoint to switch processors" << endl;

#ifdef GEM5
    // gem5 exit for checkpoint
    m5_exit(0);
#endif // GEM5

    for (uint64_t i = 0; i < num_ops; i++) {
        arr[rand() % arr_size]++;
    }

    cout << "Done with execution!" << endl;

#ifdef GEM5
    // end simulation
    m5_exit(0);
#endif // GEM5

    return 0;
};
