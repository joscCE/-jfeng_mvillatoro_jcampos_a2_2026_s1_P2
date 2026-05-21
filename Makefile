# 1. Variables
CC = gcc
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs)

# 2. Objetivo por defecto
all: main

# 3. Compilación del ejecutable
main: main.c
	$(CC) $(CFLAGS) main.c -o main $(LIBS)

# 4. Limpieza del proyecto
clean:
	rm -f main