#include <stdio.h>
#include <cuda_runtime.h>

#include "headers/gpu_cuda.h"

__global__ void hello_kernel()
{
    printf("Hello from CUDA thread %d\n", threadIdx.x);
}

int run_cuda_test(void)
{
    int device_count = 0;

    cudaError_t err =
        cudaGetDeviceCount(&device_count);

    if(err != cudaSuccess)
    {
        printf("CUDA error: %s\n",
               cudaGetErrorString(err));
        return 1;
    }

    printf("CUDA devices found: %d\n",
           device_count);

    cudaDeviceProp prop;

    cudaGetDeviceProperties(&prop, 0);

    printf("GPU: %s\n", prop.name);
    printf("SMs: %d\n",
           prop.multiProcessorCount);

    printf("Global memory: %.2f GB\n",
           prop.totalGlobalMem /
           (1024.0 * 1024.0 * 1024.0));

    hello_kernel<<<1,8>>>();

    cudaDeviceSynchronize();

    return 0;
}