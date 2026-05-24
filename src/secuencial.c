#include <stdio.h>
#include <SDL2/SDL.h>

#include "headers/secuencial.h"

// coeficientes de difusión
float Da = 1.0f;
float Db = 1.5f;


// diferencial de tiempo
float dt = 1.0f;

// entrega y desaparición
float feed = 0.055f;
float kill = 1.062f;

// matrices actuales
float A[HEIGHT][WIDTH];
float B[HEIGHT][WIDTH];

// matrices siguientes
float A_next[HEIGHT][WIDTH];
float B_next[HEIGHT][WIDTH];


//--------------------------------------------------
// Laplaciano
//--------------------------------------------------
float laplacian(int x, int y, float M[HEIGHT][WIDTH])
{
	return
		-1.0f * M[x][y] //centro

		+ 0.2f * ( //lados 
			M[x+1][y] +
			M[x-1][y] +
			M[x][y+1] +
			M[x][y-1]
		)

		+ 0.05f * ( //esquinas
			M[x+1][y+1] +
			M[x+1][y-1] +
			M[x-1][y+1] +
			M[x-1][y-1]
		);
}


//--------------------------------------------------
// Inicializar matrices
//--------------------------------------------------
void init_simulation()
{
	for(int i=0;i<HEIGHT;i++){
		for(int j=0;j<WIDTH;j++){

			A[i][j] = 1.0f;  //iniciamos todo A con 1
			B[i][j] = 0.0f; //todo B con 0

			A_next[i][j] = 1.0f; //igual para las siguientes
			B_next[i][j] = 0.0f;
		}
	}

	// gota inicial de B en el centro
	for(int i=HEIGHT/2-5; i<HEIGHT/2+5; i++){
		for(int j=WIDTH/2-5; j<WIDTH/2+5; j++){
			B[i][j] = 1.0f; //la gota que inicia todo
		}
	}
}


//--------------------------------------------------
// Un paso de simulación
//--------------------------------------------------
void simulate_step()
{
	for(int i=1;i<HEIGHT-1;i++){
		for(int j=1;j<WIDTH-1;j++){

			float reaction =
				A[i][j] * B[i][j] * B[i][j];

			float dA =
				Da * laplacian(i,j,A)
				- reaction
				+ feed*(1.0f - A[i][j]);

			float dB =
				Db * laplacian(i,j,B)
				+ reaction
				- B[i][j]*(kill + feed);

			A_next[i][j] =
				A[i][j] + dA * dt;

			B_next[i][j] =
				B[i][j] + dB * dt;

			// evitar valores fuera de rango
			if(A_next[i][j] < 0) A_next[i][j] = 0;
			if(A_next[i][j] > 1) A_next[i][j] = 1;

			if(B_next[i][j] < 0) B_next[i][j] = 0;
			if(B_next[i][j] > 1) B_next[i][j] = 1;
		}
	}

	// hacer copia al siguiente
	for(int i=0;i<HEIGHT;i++){
		for(int j=0;j<WIDTH;j++){
			A[i][j] = A_next[i][j];
			B[i][j] = B_next[i][j];
		}
	}
}


//--------------------------------------------------
// Dibujar simulación
//--------------------------------------------------
void draw(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	for(int i=0;i<HEIGHT;i++){
		for(int j=0;j<WIDTH;j++){

			int c = (int)(B[i][j] * 255.0f);

			if(c < 0) c = 0;
			if(c > 255) c = 255;

			SDL_SetRenderDrawColor(
				renderer,
				c, c, c, 255
			);

			SDL_Rect rect = {
				j * CELL_SIZE,
				i * CELL_SIZE,
				CELL_SIZE,
				CELL_SIZE
			};

			SDL_RenderFillRect(renderer, &rect);
		}
	}

	SDL_RenderPresent(renderer);
}


//--------------------------------------------------
// Main (secuencial)
//--------------------------------------------------
int run_sequential_simulation(int visual_mode)
{
	//--------------------------------------------------
	// Inicializar simulación
	//--------------------------------------------------
	init_simulation();

	if(visual_mode == 1){
		//--------------------------------------------------
		// Modo visual con SDL
		//--------------------------------------------------
		if(SDL_Init(SDL_INIT_VIDEO) < 0){
			printf("Error inicializando SDL\n");
			return 1;
		}

		SDL_Window* window = SDL_CreateWindow(
			"Reaction Diffusion",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			WINDOW_WIDTH,
			WINDOW_HEIGHT,
			0
		);

		if(window == NULL){
			printf("Error creando ventana\n");
			SDL_Quit();
			return 1;
		}

		SDL_Renderer* renderer = SDL_CreateRenderer(
			window,
			-1,
			SDL_RENDERER_ACCELERATED
		);

		if(renderer == NULL){
			printf("Error creando renderer\n");
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 1;
		}

		int running = 1;
		SDL_Event event;

		while(running){
			while(SDL_PollEvent(&event)){
				if(event.type == SDL_QUIT){
					running = 0;
				}
			}

			simulate_step();
			draw(renderer);
			SDL_Delay(16);
		}

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 0;
	}

	//--------------------------------------------------
	// Benchmark secuencial (solo computación)
	//--------------------------------------------------
	Uint64 start_counter = SDL_GetPerformanceCounter();

	for(int step=0; step<SEQ_SIM_STEPS; step++){
		simulate_step();
	}

	Uint64 end_counter = SDL_GetPerformanceCounter();
	double elapsed_seconds =
		(double)(end_counter - start_counter) /
		(double)SDL_GetPerformanceFrequency();

	printf("Simulacion secuencial completada en %d pasos\n", SEQ_SIM_STEPS);
	printf("Tiempo total: %.3f ms (%.6f s)\n", elapsed_seconds * 1000.0, elapsed_seconds);
	printf("Muestra centro -> A: %.6f, B: %.6f\n", A[HEIGHT / 2][WIDTH / 2], B[HEIGHT / 2][WIDTH / 2]);

	return 0;
}
