#include "headers/secuencial.h"
#include "headers/gpu_cuda.h"

int main(void)
{
    //return run_sequential_simulation(0); // benchmark
    //return run_sequential_simulation(1); // visual
    return run_cuda_simulation();
}