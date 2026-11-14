#ifndef BACKTRACKING_H
#define BACKTRACKING_H

#include "curso.h"

// Modulo de planeacion por backtracking (U4).
//
// Genera un plan semestre por semestre para las materias que faltan. En cada
// semestre se prueban los subconjuntos de materias elegibles que cumplan los
// topes (creditos y/o numero de materias) y sean MAXIMALES (no se puede
// agregar otra elegible sin pasarse del tope). Con cada subconjunto se
// avanza al semestre siguiente; si no hay salida, se regresa (backtrack).
//


typedef struct {
    int encontrado;               // 1 si hay al menos un plan
    int semestres;                // semestres del mejor plan encontrado
    int primer_plan_semestres;    // semestres del primer plan que hallo la busqueda
    int *asignacion;              // asignacion[i] = semestre (1..) o 0 si ya estaba aprobada
    long long pasos;              // nodos de busqueda visitados
    int truncado;                 // 1 si se corto por limite_pasos
    int optimo_demostrado;        // 1 si busqueda completa: 'semestres' es el minimo
} ResultadoPlan;

// aprobadas: arreglo de n chars (1 = aprobada). max_creditos / max_materias
// <= 0 significa sin tope. Una materia que sola excede max_creditos se cursa
// sola (igual que en el modulo voraz). Requiere n <= 64.
// Devuelve 0 si hay plan, 1 si no se hallo plan dentro del limite,
// -1 si hay ciclo o falla la memoria, -2 si n > 64.
int plan_backtracking(const Curso *cursos, int n, const char *aprobadas,
                      int max_creditos, int max_materias,
                      long long limite_pasos, ResultadoPlan *res);

void liberar_resultado_plan(ResultadoPlan *res);

// Imprime el plan semestre por semestre indicando que es exacto y que no.
void imprimir_plan_backtracking(const Curso *cursos, int n, const ResultadoPlan *res);

#endif // BACKTRACKING_H
