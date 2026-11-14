#ifndef ELEGIBLES_H
#define ELEGIBLES_H

#include "curso.h"

// Modulo de elegibilidad: dado un historial de materias aprobadas,
// calcula cuales se pueden cursar el siguiente periodo, es decir, las NO
// aprobadas que tienen TODOS sus prerrequisitos aprobados.
//
// El historial es un arreglo de n chars: aprobadas[i] = 1 si la materia i
// (indice base 0 del archivo) ya esta aprobada.
//

// Lee un historial como texto: indices separados por coma y/o rangos, por
// ejemplo "0,1,2,5-9". Devuelve cuantas materias marco, o -1 si el texto
// es invalido (indice fuera de rango o formato incorrecto).
int parsear_historial(const char *texto, int n, char *aprobadas);

// Marca como aprobadas todas las materias del plan con semestre <= sem.
void aprobar_hasta_semestre(const Curso *cursos, int n, int sem, char *aprobadas);

// Revisa que el historial sea coherente (nadie aprobo algo sin sus
// prerrequisitos). Imprime las incoherencias y devuelve cuantas hay.
int validar_historial(const Curso *cursos, int n, const char *aprobadas);

// Llena salida[] con los indices de las materias elegibles y devuelve cuantas.
int materias_elegibles(const Curso *cursos, int n, const char *aprobadas, int *salida);

#endif
