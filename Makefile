CC = gcc
SRC_DIR = src
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/secuencial.c
CUDA_SRC = $(SRC_DIR)/gpu_cuda.cu
CUDA_OBJ = gpu_cuda.obj

ifeq ($(OS),Windows_NT)
TARGET = main.exe
CFLAGS = -Wall -g -IC:/msys64/ucrt64/include/SDL2
LIBS = -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2
REMOVE = del /Q
VCVARS64 = C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat
else
TARGET = main
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs)
REMOVE = rm -f
endif

# 1. Targets que no son archivos
.PHONY: all run cuda clean

# 2. Objetivo por defecto
all: $(TARGET)

# 3. Compilación del ejecutable
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

# 4. Ejecutar binario principal
run: $(TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TARGET)
else
	./$(TARGET)
endif

# 5. Compilar prueba CUDA a objeto
cuda:
ifeq ($(OS),Windows_NT)
	cmd /C "\"$(VCVARS64)\" && nvcc -c $(CUDA_SRC) -o $(CUDA_OBJ)"
else
	nvcc -c $(CUDA_SRC) -o $(CUDA_OBJ)
endif

# 6. Limpieza del proyecto
clean:
	-$(REMOVE) main main.exe
ifeq ($(OS),Windows_NT)
	-$(REMOVE) $(CUDA_OBJ)
else
	-$(REMOVE) $(CUDA_OBJ) *.ptx *.cubin *.fatbin
endif