#include "simple-multithreader.h"
#include <assert.h>
#include <cmath>

int main(int argc, char** argv) {
    // intialize problem size
    int numThread = argc>1 ? atoi(argv[1]) : 2;
    int size = argc>2 ? atoi(argv[2]) : 4800000000;  
    // allocate vectors
    int* A = new int[size];
    int* B = new int[size];
    int* C = new int[size];
    // initialize the vectors
    std::fill(A, A+size, 1);
    std::fill(B, B+size, 1);
    std::fill(C, C+size, 0);
    // start the parallel addition of two vectors
    parallel_for(0, size,
    [&](int i) {
    double temp_a = (double)A[i];
    double temp_b = (double)B[i];
    
    for (int j = 0; j < 10000; j++) {
        temp_a = std::sin(temp_a) + std::cos(temp_b);
        temp_b = std::log(std::abs(temp_a));
    }
    
    C[i] = (int)temp_a; // Store the final result
}, numThread);
    // cleanup memory
    delete[] A;
    delete[] B;
    delete[] C;
    return 0;
}

