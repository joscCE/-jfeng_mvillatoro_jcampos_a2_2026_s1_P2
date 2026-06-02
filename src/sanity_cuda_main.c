#include <stdio.h>

#include "headers/gpu_cuda.h"

int main(int argc, char** argv)
{
    // Permite elegir archivo de salida por argumento o usar uno por defecto.
    const char* output_path = (argc > 1) ? argv[1] : "sanity_results_512x512_5000.txt";

    // Ejecuta benchmark CUDA para producir estado final en GPU y copiarlo a host.
    if (run_cuda_simulation() != 0) {
        return 1;
    }

    // Abre el reporte en modo append para sumar una linea por backend.
    FILE* out = fopen(output_path, "a");
    if (out == NULL) {
        printf("No se pudo abrir el archivo de salida: %s\n", output_path);
        return 1;
    }

    // Escribe la metrica escalar usada para sanity check.
    fprintf(out, "cuda_sum_b=%.9f\n", cuda_sum_b());
    fclose(out);

    printf("Resultado CUDA escrito en: %s\n", output_path);
    return 0;
}
