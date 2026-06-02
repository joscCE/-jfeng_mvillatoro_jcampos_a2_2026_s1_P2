#ifndef CONFIG_H
#define CONFIG_H

// Dimensiones globales de la simulación
#define HEIGHT 512
#define WIDTH 512

// Pasos para los benchmarks 
#define SIM_STEPS 5000

// Configuración visual de la ventana SDL
#define CELL_SIZE 2
#define WINDOW_WIDTH  (WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (HEIGHT * CELL_SIZE)

#endif