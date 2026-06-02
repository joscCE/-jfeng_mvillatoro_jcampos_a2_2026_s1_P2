#ifndef SIMD_AVX2_H
#define SIMD_AVX2_H

// Ejecuta la version SIMD AVX2: 0 = benchmark, 1 = visual.
int run_simd_simulation(int visual_mode);

// Suma de todos los valores de B de la ultima corrida SIMD.
double simd_last_sum_b(void);

#endif
