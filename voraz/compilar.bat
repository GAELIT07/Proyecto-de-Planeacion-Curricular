@echo off
REM Compila el modulo voraz en Windows (requiere gcc de MinGW en el PATH)
gcc -Wall -Wextra -std=c11 -g -I..\curso -I..\leer_Archivo -I..\quicksort -o voraz.exe voraz_main.c voraz.c ..\quicksort\quicksort.c ..\curso\curso.c ..\leer_Archivo\leerArchivo.c
if errorlevel 1 (echo ERROR al compilar) else (echo Listo: voraz.exe ..\quicksort\cursos.txt 30)
