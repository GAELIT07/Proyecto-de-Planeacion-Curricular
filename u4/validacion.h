#ifndef VALIDACION_H
#define VALIDACION_H

#include "curso.h"

//
// El grafo se recorre con DFS de tres colores (0 = sin visitar, 1 = en la
// pila de recursion, 2 = terminado). Si el DFS encuentra una arista hacia un
// nodo que sigue "en la pila" (arista de retorno), esa pila ES la evidencia
// del ciclo: la secuencia de materias A -> B -> ... -> A.
//

#define MAX_CICLOS 16
#define MAX_LONG_CICLO 128

typedef struct {
    int nodos[MAX_LONG_CICLO];   // nodos[0] requiere a nodos[1], ..., el ultimo requiere a nodos[0]
    int longitud;
} Ciclo;

// Cuenta (e imprime) los prerrequisitos con indice fuera de rango.
int validar_indices(const Curso *cursos, int n);

// Busca ciclos de prerrequisitos. Guarda hasta max_ciclos en 'ciclos' (uno
// por cada arista de retorno del DFS; no son TODOS los ciclos elementales,
// pero si hay alguno, al menos uno se reporta). Devuelve cuantos encontro.
// Una materia que se pide a si misma cuenta como ciclo de longitud 1.
int detectar_ciclos(const Curso *cursos, int n, Ciclo *ciclos, int max_ciclos);

// Imprime un ciclo como secuencia de nombres.
void imprimir_ciclo(const Curso *cursos, const Ciclo *ciclo);

// Orden topologico (prerrequisitos antes que las materias que los piden),
// por DFS en postorden. orden[] debe tener n lugares.
// Devuelve 0 si todo bien, -1 si hay ciclo. No es unico: es UN orden valido.
int orden_topologico(const Curso *cursos, int n, int *orden);

#endif 