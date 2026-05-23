CC = gcc
SRC_DIR = src
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/secuencial.c

ifeq ($(OS),Windows_NT)
TARGET = main.exe
CFLAGS = -Wall -g -IC:/msys64/ucrt64/include/SDL2
LIBS = -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2
REMOVE = del /Q
else
TARGET = main
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs)
REMOVE = rm -f
endif

# 2. Objetivo por defecto
all: $(TARGET)

# 3. Compilación del ejecutable
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

# 4. Limpieza del proyecto
clean:
	-$(REMOVE) main main.exe