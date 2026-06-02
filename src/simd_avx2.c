#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h> // Intrínsecos AVX2
#include <SDL2/SDL.h>

#include "headers/config.h"

// Traer las constantes físicas definidas en secuencial
extern float Da, Db, dt, feed, kill;

// Punteros dinámicos para evitar el bucle de copia manual
static float* A_ptr = NULL;
static float* B_ptr = NULL;
static float* A_next_ptr = NULL;
static float* B_next_ptr = NULL;
// Conserva la suma final de B para consultarla despues de liberar memoria.
static double last_sum_b = 0.0;

static double sum_current_b()
{
    // Si la memoria SIMD ya fue liberada, se devuelve el ultimo valor cacheado.
    if (B_ptr == NULL) {
        return last_sum_b;
    }

    // Suma escalar de toda la grilla B para comparacion entre backends.
    double sum = 0.0;
    int total = HEIGHT * WIDTH;
    for (int idx = 0; idx < total; idx++) {
        sum += (double)B_ptr[idx];
    }
    return sum;
}

//--------------------------------------------------
// Inicializar matrices asignando memoria alineada a 32 bytes (AVX2)
//--------------------------------------------------
static void init_simd_simulation()
{
    // Alinear a 32 bytes para que _mm256_load_ps y _mm256_store_ps no fallen
    size_t matrix_size = HEIGHT * WIDTH * sizeof(float);
    
    A_ptr      = (float*)_mm_malloc(matrix_size, 32);
    B_ptr      = (float*)_mm_malloc(matrix_size, 32);
    A_next_ptr = (float*)_mm_malloc(matrix_size, 32);
    B_next_ptr = (float*)_mm_malloc(matrix_size, 32);
    // Reinicia el acumulado para que cada corrida empiece limpia.
    last_sum_b = 0.0;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int idx = i * WIDTH + j;
            A_ptr[idx] = 1.0f;
            B_ptr[idx] = 0.0f;
            A_next_ptr[idx] = 1.0f;
            B_next_ptr[idx] = 0.0f;
        }
    }

    // Gota inicial de B en el centro
    for (int i = HEIGHT / 2 - 5; i < HEIGHT / 2 + 5; i++) {
        for (int j = WIDTH / 2 - 5; j < WIDTH / 2 + 5; j++) {
            B_ptr[i * WIDTH + j] = 1.0f;
        }
    }
}

//--------------------------------------------------
// Liberar memoria al terminar
//--------------------------------------------------
static void free_simd_simulation()
{
    _mm_free(A_ptr);
    _mm_free(B_ptr);
    _mm_free(A_next_ptr);
    _mm_free(B_next_ptr);

    // Deja punteros nulos para evitar usos accidentales tras free.
    A_ptr = NULL;
    B_ptr = NULL;
    A_next_ptr = NULL;
    B_next_ptr = NULL;
}

//--------------------------------------------------
// Un paso de simulación optimizado con AVX2
//--------------------------------------------------
static void simulate_step_avx2()
{
    // Precalcular constantes en registros SIMD
    __m256 v_Da   = _mm256_set1_ps(Da);
    __m256 v_Db   = _mm256_set1_ps(Db);
    __m256 v_dt   = _mm256_set1_ps(dt);
    __m256 v_feed = _mm256_set1_ps(feed);
    
    __m256 v_one  = _mm256_set1_ps(1.0f);
    __m256 v_zero = _mm256_set1_ps(0.0f);
    
    __m256 w_center   = _mm256_set1_ps(-1.0f);
    __m256 w_adjacent = _mm256_set1_ps(0.2f);
    __m256 w_diagonal = _mm256_set1_ps(0.05f);
    __m256 v_kill_plus_feed = _mm256_set1_ps(kill + feed);

    // Procesar cada celda, saltando de 8 en 8
    for (int i = 1; i < HEIGHT - 1; i++) {
        int idx_row      = i * WIDTH;
        int idx_row_up   = (i - 1) * WIDTH;
        int idx_row_down = (i + 1) * WIDTH;

        // calcula 8 celdas a la vez
        for (int j = 1; j < WIDTH - 1; j += 8) {
            
            // Manejo de remanente si la fila no termina de ajustar en bloques de 8
            if (j + 8 > WIDTH - 1) {
                for (int r = j; r < WIDTH - 1; r++) {
                    int idx = idx_row + r;
                    float reaction = A_ptr[idx] * B_ptr[idx] * B_ptr[idx];
                    
                    float lapA = -1.0f * A_ptr[idx] 
                        + 0.2f * (A_ptr[idx_row_down + r] + A_ptr[idx_row_up + r] + A_ptr[idx + 1] + A_ptr[idx - 1])
                        + 0.05f * (A_ptr[idx_row_down + r + 1] + A_ptr[idx_row_down + r - 1] + A_ptr[idx_row_up + r + 1] + A_ptr[idx_row_up + r - 1]);
                        
                    float lapB = -1.0f * B_ptr[idx] 
                        + 0.2f * (B_ptr[idx_row_down + r] + B_ptr[idx_row_up + r] + B_ptr[idx + 1] + B_ptr[idx - 1])
                        + 0.05f * (B_ptr[idx_row_down + r + 1] + B_ptr[idx_row_down + r - 1] + B_ptr[idx_row_up + r + 1] + B_ptr[idx_row_up + r - 1]);

                    A_next_ptr[idx] = A_ptr[idx] + (Da * lapA - reaction + feed * (1.0f - A_ptr[idx])) * dt;
                    B_next_ptr[idx] = B_ptr[idx] + (Db * lapB + reaction - B_ptr[idx] * (kill + feed)) * dt;

                    if (A_next_ptr[idx] < 0) A_next_ptr[idx] = 0;
                    if (A_next_ptr[idx] > 1) A_next_ptr[idx] = 1;
                    if (B_next_ptr[idx] < 0) B_next_ptr[idx] = 0;
                    if (B_next_ptr[idx] > 1) B_next_ptr[idx] = 1;
                }
                break;
            }

            // --- PROCESAR COMPONENTE A ---

            // Cargar el centro, celdas j, j+1, ..., j+7
            __m256 a_center = _mm256_loadu_ps(&A_ptr[idx_row + j]);
            // Cargar vecinos, cada uno también con 8 celdas contiguas
            __m256 a_up     = _mm256_loadu_ps(&A_ptr[idx_row_up + j]);
            __m256 a_down   = _mm256_loadu_ps(&A_ptr[idx_row_down + j]);
            __m256 a_left   = _mm256_loadu_ps(&A_ptr[idx_row + j - 1]);
            __m256 a_right  = _mm256_loadu_ps(&A_ptr[idx_row + j + 1]);
            
            __m256 a_tl     = _mm256_loadu_ps(&A_ptr[idx_row_up + j - 1]); // top left
            __m256 a_tr     = _mm256_loadu_ps(&A_ptr[idx_row_up + j + 1]); // top right
            __m256 a_bl     = _mm256_loadu_ps(&A_ptr[idx_row_down + j - 1]); // bottom left
            __m256 a_br     = _mm256_loadu_ps(&A_ptr[idx_row_down + j + 1]); /// bottom right

            // Calcular laplaciano para A usando la máscara de convolución
            __m256 lapA = _mm256_mul_ps(a_center, w_center);
            __m256 sum_adjA = _mm256_add_ps(_mm256_add_ps(a_up, a_down), _mm256_add_ps(a_left, a_right)); // Sumar 4 lados
            lapA = _mm256_add_ps(lapA, _mm256_mul_ps(sum_adjA, w_adjacent)); 
            __m256 sum_diagA = _mm256_add_ps(_mm256_add_ps(a_tl, a_tr), _mm256_add_ps(a_bl, a_br)); // Sumar 4 diagonales
            lapA = _mm256_add_ps(lapA, _mm256_mul_ps(sum_diagA, w_diagonal)); 

            // --- PROCESAR COMPONENTE B ---

            // Cargar el centro y vecinos para B
            __m256 b_center = _mm256_loadu_ps(&B_ptr[idx_row + j]);
            __m256 b_up     = _mm256_loadu_ps(&B_ptr[idx_row_up + j]);
            __m256 b_down   = _mm256_loadu_ps(&B_ptr[idx_row_down + j]);
            __m256 b_left   = _mm256_loadu_ps(&B_ptr[idx_row + j - 1]);
            __m256 b_right  = _mm256_loadu_ps(&B_ptr[idx_row + j + 1]);
            
            __m256 b_tl     = _mm256_loadu_ps(&B_ptr[idx_row_up + j - 1]);
            __m256 b_tr     = _mm256_loadu_ps(&B_ptr[idx_row_up + j + 1]);
            __m256 b_bl     = _mm256_loadu_ps(&B_ptr[idx_row_down + j - 1]);
            __m256 b_br     = _mm256_loadu_ps(&B_ptr[idx_row_down + j + 1]);

            __m256 lapB = _mm256_mul_ps(b_center, w_center);
            __m256 sum_adjB = _mm256_add_ps(_mm256_add_ps(b_up, b_down), _mm256_add_ps(b_left, b_right));
            lapB = _mm256_add_ps(lapB, _mm256_mul_ps(sum_adjB, w_adjacent));
            __m256 sum_diagB = _mm256_add_ps(_mm256_add_ps(b_tl, b_tr), _mm256_add_ps(b_bl, b_br));
            lapB = _mm256_add_ps(lapB, _mm256_mul_ps(sum_diagB, w_diagonal));

            // --- REACCIÓN GRUPO ---

            // Reaccion = A * B^2 para 8 celdas
            __m256 reaction = _mm256_mul_ps(a_center, _mm256_mul_ps(b_center, b_center));

            // dA = Da * lapA - reaction + feed * (1.0f - A)
            __m256 dA = _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(v_Da, lapA), reaction), _mm256_mul_ps(v_feed, _mm256_sub_ps(v_one, a_center)));
            // dB = Db * lapB + reaction - B * (kill + feed)
            __m256 dB = _mm256_sub_ps(_mm256_add_ps(_mm256_mul_ps(v_Db, lapB), reaction), _mm256_mul_ps(b_center, v_kill_plus_feed));

            // Euler: nuevo + viejo + d * dt
            __m256 a_next_val = _mm256_add_ps(a_center, _mm256_mul_ps(dA, v_dt));
            __m256 b_next_val = _mm256_add_ps(b_center, _mm256_mul_ps(dB, v_dt));

            // Clamping [0.0, 1.0]
            a_next_val = _mm256_max_ps(v_zero, _mm256_min_ps(v_one, a_next_val));
            b_next_val = _mm256_max_ps(v_zero, _mm256_min_ps(v_one, b_next_val));

            // Escribir los 8 resultados de vuelta a memoria
            _mm256_storeu_ps(&A_next_ptr[idx_row + j], a_next_val);
            _mm256_storeu_ps(&B_next_ptr[idx_row + j], b_next_val);
        }
    }

    float* tempA = A_ptr; A_ptr = A_next_ptr; A_next_ptr = tempA;
    float* tempB = B_ptr; B_ptr = B_next_ptr; B_next_ptr = tempB;
}

//--------------------------------------------------
// Dibujar con SDL desde los punteros SIMD
//--------------------------------------------------
static void draw_simd(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int c = (int)(B_ptr[i * WIDTH + j] * 255.0f);
            if (c < 0) c = 0;
            if (c > 255) c = 255;

            SDL_SetRenderDrawColor(renderer, c, c, c, 255);
            SDL_Rect rect = { j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_RenderPresent(renderer);
}

//--------------------------------------------------
// Función Principal del Módulo SIMD
//--------------------------------------------------
int run_simd_simulation(int visual_mode)
{
    init_simd_simulation();

    if (visual_mode == 1) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
        SDL_Window* window = SDL_CreateWindow("Reaction Diffusion - AVX2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
        if (!window) { SDL_Quit(); return 1; }
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }

        int running = 1;
        SDL_Event event;
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = 0;
            }
            simulate_step_avx2();
            draw_simd(renderer);
            SDL_Delay(16);
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        // Guarda la suma final antes de liberar buffers SIMD.
        last_sum_b = sum_current_b();
        free_simd_simulation();
        return 0;
    }

    // MODO BENCHMARK
    Uint64 start_counter = SDL_GetPerformanceCounter();
    for (int step = 0; step < SIM_STEPS; step++) {
        simulate_step_avx2();
    }
    Uint64 end_counter = SDL_GetPerformanceCounter();
    
    double elapsed_seconds = (double)(end_counter - start_counter) / (double)SDL_GetPerformanceFrequency();
    // Guarda la suma final para que el caller la escriba en el reporte.
    last_sum_b = sum_current_b();
    printf("\n--- SIMULACIÓN SIMD AVX2 COMPLETADA (%d pasos) ---\n", SIM_STEPS);
    printf("Tiempo de cómputo: %.3f ms (%.6f s)\n", elapsed_seconds * 1000.0, elapsed_seconds);
    printf("Muestra centro -> A: %.6f, B: %.6f\n", A_ptr[(HEIGHT/2)*WIDTH + (WIDTH/2)], B_ptr[(HEIGHT/2)*WIDTH + (WIDTH/2)]);

    free_simd_simulation();
    return 0;
}

double simd_last_sum_b(void)
{
    // API publica para leer el ultimo resultado agregado por SIMD.
    return last_sum_b;
}

