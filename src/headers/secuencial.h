#ifndef SECUENCIAL_H
#define SECUENCIAL_H

// tamaño de la matriz
#define HEIGHT 300
#define WIDTH 300

// pasos para benchmark secuencial
#define SEQ_SIM_STEPS 500

// tamaño visual de cada celda
#define CELL_SIZE 2

// tamaño ventana
#define WINDOW_WIDTH (WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (HEIGHT * CELL_SIZE)

// ejecuta la versión secuencial: 0 = benchmark, 1 = visual
int run_sequential_simulation(int visual_mode);

#endif
