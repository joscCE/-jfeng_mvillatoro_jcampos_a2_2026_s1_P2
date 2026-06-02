#include <stdio.h>
#include <cuda_runtime.h>

#include "headers/gpu_cuda.h"

// coeficientes de difusion
float Da = GPU_DA;
float Db = GPU_DB;

// diferencial de tiempo
float dt = GPU_DT;

// entrega y desaparicion
float feed = GPU_FEED;
float kill = GPU_KILL;

// matrices actuales
float A[GPU_HEIGHT][GPU_WIDTH];
float B[GPU_HEIGHT][GPU_WIDTH];

// matrices siguientes
float A_next[GPU_HEIGHT][GPU_WIDTH];
float B_next[GPU_HEIGHT][GPU_WIDTH];

// memoria de GPU
static float* d_A = NULL;
static float* d_B = NULL;
static float* d_A_next = NULL;
static float* d_B_next = NULL;

static int check_cuda(cudaError_t err, const char* where)
{
	if (err != cudaSuccess) {
		printf("CUDA error en %s: %s\n", where, cudaGetErrorString(err));
		return 0;
	}
	return 1;
}

__global__ void update_step_kernel(
	const float* A_cur,
	const float* B_cur,
	float* A_nxt,
	float* B_nxt,
	float Da_v,
	float Db_v,
	float dt_v,
	float feed_v,
	float kill_v)
{
	int i = blockIdx.y * blockDim.y + threadIdx.y;
	int j = blockIdx.x * blockDim.x + threadIdx.x;

	if (i <= 0 || i >= GPU_HEIGHT - 1 || j <= 0 || j >= GPU_WIDTH - 1) {
		return;
	}

	int idx = i * GPU_WIDTH + j;

	float Aij = A_cur[idx];
	float Bij = B_cur[idx];

	float lapA =
		-1.0f * Aij
		+ 0.2f * (
			A_cur[(i + 1) * GPU_WIDTH + j] +
			A_cur[(i - 1) * GPU_WIDTH + j] +
			A_cur[i * GPU_WIDTH + (j + 1)] +
			A_cur[i * GPU_WIDTH + (j - 1)])
		+ 0.05f * (
			A_cur[(i + 1) * GPU_WIDTH + (j + 1)] +
			A_cur[(i + 1) * GPU_WIDTH + (j - 1)] +
			A_cur[(i - 1) * GPU_WIDTH + (j + 1)] +
			A_cur[(i - 1) * GPU_WIDTH + (j - 1)]);

	float lapB =
		-1.0f * Bij
		+ 0.2f * (
			B_cur[(i + 1) * GPU_WIDTH + j] +
			B_cur[(i - 1) * GPU_WIDTH + j] +
			B_cur[i * GPU_WIDTH + (j + 1)] +
			B_cur[i * GPU_WIDTH + (j - 1)])
		+ 0.05f * (
			B_cur[(i + 1) * GPU_WIDTH + (j + 1)] +
			B_cur[(i + 1) * GPU_WIDTH + (j - 1)] +
			B_cur[(i - 1) * GPU_WIDTH + (j + 1)] +
			B_cur[(i - 1) * GPU_WIDTH + (j - 1)]);

	float reaction = Aij * Bij * Bij;

	float dA = Da_v * lapA - reaction + feed_v * (1.0f - Aij);
	float dB = Db_v * lapB + reaction - Bij * (kill_v + feed_v);

	float newA = Aij + dA * dt_v;
	float newB = Bij + dB * dt_v;

	if (newA < 0.0f) newA = 0.0f;
	if (newA > 1.0f) newA = 1.0f;
	if (newB < 0.0f) newB = 0.0f;
	if (newB > 1.0f) newB = 1.0f;

	A_nxt[idx] = newA;
	B_nxt[idx] = newB;
}

__global__ void copy_step_kernel(float* A_cur, float* B_cur, const float* A_nxt, const float* B_nxt)
{
	int i = blockIdx.y * blockDim.y + threadIdx.y;
	int j = blockIdx.x * blockDim.x + threadIdx.x;

	if (i >= GPU_HEIGHT || j >= GPU_WIDTH) {
		return;
	}

	int idx = i * GPU_WIDTH + j;
	A_cur[idx] = A_nxt[idx];
	B_cur[idx] = B_nxt[idx];
}

//--------------------------------------------------
// Inicializar matrices
//--------------------------------------------------
void init_simulation()
{
	for (int i = 0; i < GPU_HEIGHT; i++) {
		for (int j = 0; j < GPU_WIDTH; j++) {
			A[i][j] = 1.0f;
			B[i][j] = 0.0f;
			A_next[i][j] = 1.0f;
			B_next[i][j] = 0.0f;
		}
	}

	for (int i = GPU_HEIGHT / 2 - 5; i < GPU_HEIGHT / 2 + 5; i++) {
		for (int j = GPU_WIDTH / 2 - 5; j < GPU_WIDTH / 2 + 5; j++) {
			B[i][j] = 1.0f;
		}
	}
}

static int prepare_device_buffers()
{
	size_t bytes = (size_t)GPU_HEIGHT * (size_t)GPU_WIDTH * sizeof(float);

	if (!check_cuda(cudaMalloc((void**)&d_A, bytes), "cudaMalloc d_A")) return 0;
	if (!check_cuda(cudaMalloc((void**)&d_B, bytes), "cudaMalloc d_B")) return 0;
	if (!check_cuda(cudaMalloc((void**)&d_A_next, bytes), "cudaMalloc d_A_next")) return 0;
	if (!check_cuda(cudaMalloc((void**)&d_B_next, bytes), "cudaMalloc d_B_next")) return 0;

	if (!check_cuda(cudaMemcpy(d_A, A, bytes, cudaMemcpyHostToDevice), "copy A H2D")) return 0;
	if (!check_cuda(cudaMemcpy(d_B, B, bytes, cudaMemcpyHostToDevice), "copy B H2D")) return 0;
	if (!check_cuda(cudaMemcpy(d_A_next, A_next, bytes, cudaMemcpyHostToDevice), "copy A_next H2D")) return 0;
	if (!check_cuda(cudaMemcpy(d_B_next, B_next, bytes, cudaMemcpyHostToDevice), "copy B_next H2D")) return 0;

	return 1;
}

static void free_device_buffers()
{
	if (d_A) cudaFree(d_A);
	if (d_B) cudaFree(d_B);
	if (d_A_next) cudaFree(d_A_next);
	if (d_B_next) cudaFree(d_B_next);
	d_A = NULL;
	d_B = NULL;
	d_A_next = NULL;
	d_B_next = NULL;
}

//--------------------------------------------------
// Un paso de simulacion (CUDA)
//--------------------------------------------------
void simulate_step()
{
	dim3 block(16, 16);
	dim3 grid((GPU_WIDTH + block.x - 1) / block.x, (GPU_HEIGHT + block.y - 1) / block.y);

	update_step_kernel<<<grid, block>>>(d_A, d_B, d_A_next, d_B_next, Da, Db, dt, feed, kill);
	copy_step_kernel<<<grid, block>>>(d_A, d_B, d_A_next, d_B_next);
}

int run_cuda_simulation(void)
{
	int device_count = 0;
	if (!check_cuda(cudaGetDeviceCount(&device_count), "cudaGetDeviceCount")) {
		return 1;
	}

	if (device_count <= 0) {
		printf("No se detectaron GPUs CUDA.\n");
		return 1;
	}

	cudaDeviceProp prop;
	if (!check_cuda(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties")) {
		return 1;
	}

	printf("CUDA devices found: %d\n", device_count);
	printf("GPU: %s\n", prop.name);
	printf("SMs: %d\n", prop.multiProcessorCount);
	printf("Global memory: %.2f GB\n", prop.totalGlobalMem / (1024.0 * 1024.0 * 1024.0));

	init_simulation();

	if (!prepare_device_buffers()) {
		free_device_buffers();
		return 1;
	}

	cudaEvent_t start_event;
	cudaEvent_t stop_event;
	if (!check_cuda(cudaEventCreate(&start_event), "cudaEventCreate start")) {
		free_device_buffers();
		return 1;
	}
	if (!check_cuda(cudaEventCreate(&stop_event), "cudaEventCreate stop")) {
		cudaEventDestroy(start_event);
		free_device_buffers();
		return 1;
	}

	check_cuda(cudaEventRecord(start_event), "cudaEventRecord start");
	for (int step = 0; step < GPU_SIM_STEPS; step++) {
		simulate_step();
	}
	if (!check_cuda(cudaGetLastError(), "kernel launch")) {
		cudaEventDestroy(start_event);
		cudaEventDestroy(stop_event);
		free_device_buffers();
		return 1;
	}
	check_cuda(cudaEventRecord(stop_event), "cudaEventRecord stop");
	check_cuda(cudaEventSynchronize(stop_event), "cudaEventSynchronize stop");

	float elapsed_ms = 0.0f;
	check_cuda(cudaEventElapsedTime(&elapsed_ms, start_event, stop_event), "cudaEventElapsedTime");

	size_t bytes = (size_t)GPU_HEIGHT * (size_t)GPU_WIDTH * sizeof(float);
	check_cuda(cudaMemcpy(A, d_A, bytes, cudaMemcpyDeviceToHost), "copy A D2H");
	check_cuda(cudaMemcpy(B, d_B, bytes, cudaMemcpyDeviceToHost), "copy B D2H");

	printf("Simulacion CUDA completada en %d pasos\n", GPU_SIM_STEPS);
	printf("Tiempo total: %.3f ms (%.6f s)\n", elapsed_ms, elapsed_ms / 1000.0f);
	printf("Muestra centro -> A: %.6f, B: %.6f\n", A[GPU_HEIGHT / 2][GPU_WIDTH / 2], B[GPU_HEIGHT / 2][GPU_WIDTH / 2]);

	cudaEventDestroy(start_event);
	cudaEventDestroy(stop_event);
	free_device_buffers();

	return 0;
}

double cuda_sum_b(void)
{
	// Acumula B en host despues de copiar desde la GPU para comparar
	// contra los resultados secuencial y SIMD.
	double sum = 0.0;

	for (int i = 0; i < GPU_HEIGHT; i++) {
		for (int j = 0; j < GPU_WIDTH; j++) {
			sum += (double)B[i][j];
		}
	}

	return sum;
}
