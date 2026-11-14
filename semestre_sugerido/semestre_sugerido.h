#ifndef SEMESTRE_SUGERIDO_H
#define SEMESTRE_SUGERIDO_H

#include "curso.h"

// Modulo de semestre sugerido: asigna a cada materia un semestre cuidando
//   - los prerrequisitos (siempre en un semestre anterior),
//   - un tope de creditos por semestre,
//   - un tope de dificultad total por semestre.
// Un tope <= 0 significa "sin limite".
//
// Las materias se ordenan con quicksort (comparar_restricciones) y se van
// acomodando en ese orden. Una materia nunca se adelanta antes del semestre
// que tiene en el plan original; solo se retrasa si no cabe o si sus
// prerrequisitos todavia no estan cursados.
//
// Llena asignacion[i] (arreglo de n enteros) con el semestre sugerido del
// curso i del arreglo original, y devuelve el numero de semestres usados.
// Devuelve -1 si hay un ciclo de prerrequisitos.
int semestre_sugerido(Curso *cursos, int n, int max_creditos,
                      int max_dificultad, int *asignacion);

// Imprime el plan por semestre con su carga (creditos y dificultad).
// Las materias marcadas con '*' quedaron en un semestre distinto al del plan.
void imprimir_plan(const Curso *cursos, int n, const int *asignacion,
                   int num_semestres);

#endif // SEMESTRE_SUGERIDO_H
