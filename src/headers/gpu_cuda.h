#ifndef GPU_CUDA_H
#define GPU_CUDA_H

// configuracion de simulacion
#define GPU_HEIGHT 512
#define GPU_WIDTH 512

// pasos para benchmark
#define GPU_SIM_STEPS 5000

// parametros del modelo Gray-Scott
#define GPU_DA 1.0f
#define GPU_DB 1.5f
#define GPU_DT 1.0f
#define GPU_FEED 0.055f
#define GPU_KILL 1.062f

#ifdef __cplusplus
extern "C" {
#endif

int run_cuda_simulation(void);
// Devuelve la suma total de la malla B copiada desde GPU al finalizar la corrida.
double cuda_sum_b(void);

#ifdef __cplusplus
}
#endif

#endif