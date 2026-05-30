#include "headers/secuencial.h"
//#include "headers/gpu_cuda.h"

int run_simd_simulation(int visual_mode);

int main(void)
{
    //return run_sequential_simulation(0); // secuencial benchmark
    //return run_sequential_simulation(1); // secuencial visual

    return run_simd_simulation(0); // simd benchmark
    //return run_simd_simulation(1); // simd visual

    //return run_cuda_simulation();
}