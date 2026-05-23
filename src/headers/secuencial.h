#ifndef SECUENCIAL_H
#define SECUENCIAL_H

// tamaño de la matriz
#define HEIGHT 300
#define WIDTH 300

// tamaño visual de cada celda
#define CELL_SIZE 2

// tamaño ventana
#define WINDOW_WIDTH (WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (HEIGHT * CELL_SIZE)

// ejecuta la versión secuencial
int run_sequential_simulation(void);

#endif
