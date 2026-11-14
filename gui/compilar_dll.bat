@echo off
REM Compila el puente entre la interfaz de Python y los modulos en C.
REM Genera cursos.dll junto a main.py (requiere gcc de MinGW en el PATH).
REM
REM El main() de pruebas del quicksort esta en quicksort\quicksort_main.c
REM y no se incluye en la biblioteca.

setlocal
set INC=-I. -I..\curso -I..\leer_Archivo -I..\quicksort -I..\restricciones -I..\semestre_sugerido -I..\voraz -I..\u4

gcc -shared -static -O2 -Wall -Wextra -std=c11 %INC% -o cursos.dll ^
  puente.c ^
  ..\curso\curso.c ^
  ..\leer_Archivo\leerArchivo.c ^
  ..\restricciones\restricciones.c ^
  ..\semestre_sugerido\semestre_sugerido.c ^
  ..\voraz\voraz.c ^
  ..\u4\validacion.c ^
  ..\u4\elegibles.c ^
  ..\u4\backtracking.c ^
  ..\quicksort\quicksort.c

if errorlevel 1 (
    echo ERROR al compilar cursos.dll
) else (
    echo Listo: cursos.dll   ^(ahora ejecuta: python main.py^)
)
endlocal
