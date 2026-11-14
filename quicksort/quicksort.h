#ifndef QUICKSORT_H
#define QUICKSORT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quicksort.h"
#include "curso.h"
#include "leerArchivo.h"

// Campos por los que se puede ordenar
typedef enum {
    CRIT_CREDITOS,
    CRIT_DIFICULTAD,
    CRIT_SEMESTRE,
    CRIT_NOMBRE
} CampoCriterio;

// Un criterio de ordenamiento: campo y sentido (1 = de mayor a menor)
typedef struct {
    CampoCriterio campo;
    int descendente;
} Criterio;

#define MAX_CRITERIOS 4

// Comparadores simples
int comparar_dificultad(const void *a, const void *b);
int comparar_semestre(const void *a, const void *b);
int comparar_creditos(const void *a, const void *b);
int comparar_nombre(const void *a, const void *b);

// Comparador por varios criterios: primero se configuran con
// configurar_criterios() y luego se pasa comparar_compuesto a quicksort().
// Los empates se resuelven por nombre.
void configurar_criterios(const Criterio *criterios, int n);
int comparar_compuesto(const void *a, const void *b);

// Prioridad a las materias REPROBADAS: primero se configuran con
// configurar_reprobadas() y luego se pasa comparar_con_prioridad a quicksort().
// Las reprobadas van antes que las demas; dentro de cada grupo se aplican los
// criterios de configurar_criterios() (y los empates se resuelven por nombre).
// reprobadas[k] = 1 si la materia con indice original k esta reprobada, por lo
// que los cursos deben traer llenado el campo 'indice' (calcular_niveles o el
// puente lo hacen). configurar_reprobadas(NULL, 0) libera la lista.
void configurar_reprobadas(const char *reprobadas, int n);
int comparar_con_prioridad(const void *a, const void *b);

// Declaración de funciones
void quicksort(Curso *arreglo, int n, int (*comparador)(const void *, const void *));
void quicksort_rec(Curso *arreglo, int inicio, int fin,int (*comparador)(const void *, const void *));

int particion(Curso *arreglo, int bajo, int alto, int (*comparador)(const void *, const void *));
void swap(Curso *a, Curso *b);

// Quicksort sobre un arreglo de indices. cmp(a, b, ctx) devuelve < 0 si el
// indice a debe ir antes que b, 0 si empatan y > 0 si va despues.
typedef int (*ComparadorIdx)(int a, int b, const void *ctx);
void quicksort_indices(int *idx, int n, ComparadorIdx cmp, const void *ctx);

#endif 