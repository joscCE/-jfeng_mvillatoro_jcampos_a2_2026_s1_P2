@echo off
setlocal

REM Directorio raiz del proyecto.
set "ROOT=%~dp0"
REM Primer argumento opcional: ruta del reporte de salida.
set "OUT=%~1"

if "%OUT%"=="" set "OUT=%ROOT%sanity_results_512x512_5000.txt"

REM Limpia reporte previo para iniciar una corrida fresca.
del /q "%OUT%" >nul 2>&1

REM Encabezado del reporte con parametros del experimento.
echo Gray-Scott sanity check (512x512, 5000 pasos)> "%OUT%"
echo Parametros: Da=1.0 Db=1.5 dt=1.0 feed=0.055 kill=1.062>> "%OUT%"
echo.>> "%OUT%"

REM Habilita DLLs de MSYS2 necesarias para SDL2 en gcc.
set "PATH=C:\msys64\ucrt64\bin;%PATH%"

echo [1/3] Compilando y ejecutando secuencial...
REM Compila ejecutable de sanity secuencial y agrega su suma al txt.
gcc -Wall -g "%ROOT%src\sanity_seq_main.c" "%ROOT%src\secuencial.c" -o "%ROOT%sanity_seq.exe" -IC:/msys64/ucrt64/include -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2
if errorlevel 1 goto fail
"%ROOT%sanity_seq.exe" "%OUT%"
if errorlevel 1 goto fail

echo [2/3] Compilando y ejecutando SIMD AVX2...
REM Compila ejecutable de sanity SIMD y agrega su suma al txt.
gcc -Wall -g -O3 -mavx2 -mfma "%ROOT%src\sanity_simd_main.c" "%ROOT%src\secuencial.c" "%ROOT%src\simd_avx2.c" -o "%ROOT%sanity_simd.exe" -IC:/msys64/ucrt64/include -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2
if errorlevel 1 goto fail
"%ROOT%sanity_simd.exe" "%OUT%"
if errorlevel 1 goto fail

echo [3/3] Compilando y ejecutando CUDA...
REM Carga entorno de MSVC requerido por nvcc.
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto fail
REM Compila ejecutable de sanity CUDA y agrega su suma al txt.
nvcc "%ROOT%src\sanity_cuda_main.c" "%ROOT%src\gpu_cuda.cu" -o "%ROOT%sanity_cuda.exe"
if errorlevel 1 goto fail
"%ROOT%sanity_cuda.exe" "%OUT%"
if errorlevel 1 goto fail

echo.
echo Sanity check completado.
echo Resultados en: "%OUT%"
goto cleanup

:fail
echo.
echo Error durante sanity check.

:cleanup
REM Limpia ejecutables temporales generados para el sanity check.
del /q "%ROOT%sanity_seq.exe" >nul 2>&1
del /q "%ROOT%sanity_simd.exe" >nul 2>&1
del /q "%ROOT%sanity_cuda.exe" >nul 2>&1

endlocal
