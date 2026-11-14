#ifndef RESTRICCIONES_H
#define RESTRICCIONES_H

#include "curso.h"

// Modulo de restricciones: ordena las materias segun sus prerrequisitos.
//
// Los prerrequisitos de cada curso son INDICES (base 0) sobre el arreglo
// original que devuelve leer_cursos_archivo(). Por eso primero se calculan
// los niveles sobre ese arreglo original, y despues se ordena una copia con
// quicksort, igual que con los demas comparadores:
//
//     calcular_niveles(cursos, n);
//     Curso *copia = copiar_arreglo_cursos(cursos, n);
//     quicksort(copia, n, comparar_restricciones);

// Calcula el nivel de cada curso: 0 si no tiene prerrequisitos, y
// 1 + el nivel mas alto de sus prerrequisitos en otro caso. Tambien guarda
// en cada curso su indice original. Los prerrequisitos fuera de rango o que
// apuntan al mismo curso se ignoran.
// Devuelve 0 si todo bien, o -1 si hay un ciclo de prerrequisitos.
int calcular_niveles(Curso *cursos, int n);

// Comparador para quicksort: primero por nivel (los prerrequisitos van
// antes), despues por semestre del plan y por ultimo por nombre.
// Requiere haber llamado antes a calcular_niveles.
int comparar_restricciones(const void *a, const void *b);

#endif // RESTRICCIONES_H
