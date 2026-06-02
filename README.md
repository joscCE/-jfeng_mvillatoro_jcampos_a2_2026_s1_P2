# -jfeng_mvillatoro_jcampos_a2_2026_s1_P2

### Modelo de Reacción-Difusión (Gray-Scott)

Las ecuaciones que gobiernan la evolución de las concentraciones de las sustancias $A$ y $B$ en el sistema son:

**Para la sustancia $A$:**
$$ \frac{\partial A}{\partial t} = D_A \nabla^2 A - AB^2 + f(1 - A) $$

**Para la sustancia $B$:**
$$ \frac{\partial B}{\partial t} = D_B \nabla^2 B + AB^2 - (k + f)B $$


Biblioteca necesaria:

sudo apt install SDL2-devel

tambien compilarla

gcc main.c -o main $(sdl2-config --cflags --libs)

o con el make

Para version SIMD
gcc -O3 -mavx2 -mfma -fPIE src/simd_avx2.c src/secuencial.c -o simd -lSDL2

perf stat -e fp_arith_inst_retired.256b_packed_single ./simd

### Sanity check (secuencial vs SIMD AVX2 vs CUDA)

Se agrego un flujo manual para correr los 3 metodos con los mismos parametros
de Gray-Scott (512x512, 5000 pasos), sumar todas las celdas de la matriz B final
y escribir los resultados en un archivo `.txt`.

En Windows, ejecutar desde la raiz del proyecto:

```bat
sanity_check.bat
```

Opcionalmente puedes indicar un archivo de salida:

```bat
sanity_check.bat mi_reporte.txt
```

El script compila y ejecuta en este orden:
1. secuencial
2. SIMD AVX2
3. CUDA

Al finalizar, el archivo de salida contiene una linea por backend con el valor:

```text
secuencial_sum_b=...
simd_avx2_sum_b=...
cuda_sum_b=...
```