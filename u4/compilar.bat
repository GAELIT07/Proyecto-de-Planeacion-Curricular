@echo off
gcc -Wall -Wextra -std=c11 -g -I../curso -I../leer_Archivo -I../quicksort -I../voraz -o u4.exe u4_main.c validacion.c elegibles.c backtracking.c ../voraz/voraz.c ../quicksort/quicksort.c ../curso/curso.c ../leer_Archivo/leerArchivo.c
