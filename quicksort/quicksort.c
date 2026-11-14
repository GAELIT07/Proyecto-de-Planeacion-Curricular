#include "quicksort.h"

// Comparar por dificultad
int comparar_dificultad(const void *a, const void *b) {
    Curso *c1 = (Curso *)a;
    Curso *c2 = (Curso *)b;
    return c1->dificultad - c2->dificultad;
}
// Comparar por semestre
int comparar_semestre(const void *a, const void *b) {
    Curso *c1 = (Curso *)a;
    Curso *c2 = (Curso *)b;
    return c1->semestre - c2->semestre;
}

// Comparar por creditos
int comparar_creditos(const void *a, const void *b) {
    Curso *c1 = (Curso *)a;
    Curso *c2 = (Curso *)b;
    return c1->creditos - c2->creditos;
}

// Comparar por nombre
int comparar_nombre(const void *a, const void *b) {
    Curso *c1 = (Curso *)a;
    Curso *c2 = (Curso *)b;
    return strcmp(c1->nombre, c2->nombre);
}

// Comparar por varios criterios (ver configurar_criterios en quicksort.h)
static Criterio criterios_activos[MAX_CRITERIOS];
static int num_criterios_activos = 0;

void configurar_criterios(const Criterio *criterios, int n) {
    if (n > MAX_CRITERIOS) {
        n = MAX_CRITERIOS;
    }
    for (int i = 0; i < n; i++) {
        criterios_activos[i] = criterios[i];
    }
    num_criterios_activos = n;
}

static int comparar_campo(const Curso *c1, const Curso *c2, CampoCriterio campo) {
    switch (campo) {
        case CRIT_CREDITOS:   return (c1->creditos > c2->creditos) - (c1->creditos < c2->creditos);
        case CRIT_DIFICULTAD: return (c1->dificultad > c2->dificultad) - (c1->dificultad < c2->dificultad);
        case CRIT_SEMESTRE:   return (c1->semestre > c2->semestre) - (c1->semestre < c2->semestre);
        case CRIT_NOMBRE:     return strcmp(c1->nombre, c2->nombre);
    }
    return 0;
}

int comparar_compuesto(const void *a, const void *b) {
    const Curso *c1 = (const Curso *)a;
    const Curso *c2 = (const Curso *)b;

    for (int i = 0; i < num_criterios_activos; i++) {
        int r = comparar_campo(c1, c2, criterios_activos[i].campo);
        if (criterios_activos[i].descendente) {
            r = -r;
        }
        if (r != 0) {
            return r;
        }
    }
    return strcmp(c1->nombre, c2->nombre);
}

// Comparar dando prioridad a las materias reprobadas (ver quicksort.h)
static char *reprobadas_activas = NULL;
static int num_reprobadas_activas = 0;

void configurar_reprobadas(const char *reprobadas, int n) {
    free(reprobadas_activas);
    reprobadas_activas = NULL;
    num_reprobadas_activas = 0;

    if (reprobadas == NULL || n <= 0) {
        return;
    }
    reprobadas_activas = (char *)malloc((size_t)n);
    if (reprobadas_activas == NULL) {
        return;   // sin memoria: se ordena sin prioridad
    }
    memcpy(reprobadas_activas, reprobadas, (size_t)n);
    num_reprobadas_activas = n;
}

static int esta_reprobada(const Curso *c) {
    return c->indice >= 0 && c->indice < num_reprobadas_activas &&
           reprobadas_activas[c->indice] != 0;
}

int comparar_con_prioridad(const void *a, const void *b) {
    int r1 = esta_reprobada((const Curso *)a);
    int r2 = esta_reprobada((const Curso *)b);

    if (r1 != r2) {
        return r2 - r1;   // la reprobada (1) va antes que la que no lo esta (0)
    }
    return comparar_compuesto(a, b);
}

// QUICKSORT

void quicksort(Curso *arreglo, int n,
               int (*comparador)(const void *, const void *)) {
    if (n > 0) {
        quicksort_rec(arreglo, 0, n - 1, comparador);
    }
}

void quicksort_rec(Curso *arreglo, int inicio, int fin,
                   int (*comparador)(const void *, const void *)) {
    if (inicio < fin) {
        int indice_pivote = particion(arreglo, inicio, fin, comparador);

        quicksort_rec(arreglo, inicio, indice_pivote - 1, comparador);
        quicksort_rec(arreglo, indice_pivote + 1, fin, comparador);
    }
}


int particion(Curso *arreglo, int inicio, int fin, 
    int (*comparador)(const void *, const void *)) {
        // Elegir el ultimo elemento como pivote
    Curso pivote = arreglo[fin];
        // Indice del elemento más pequeño
    int i = inicio - 1;

    for (int j = inicio; j < fin; j++) {
        if (comparador(&arreglo[j], &pivote) <= 0) {
            i++;
            // Intercambiar
            swap(&arreglo[i], &arreglo[j]);
        }
    }

    // Intercambiar pivote a su posición final
    swap(&arreglo[i + 1], &arreglo[fin]);

    return i + 1;
}

void swap(Curso *a, Curso *b){
    Curso temp = *a;
    *a = *b;
    *b = temp;
}

// QUICKSORT SOBRE INDICES
// Ordena un arreglo de enteros (indices de cursos) con un comparador que
// recibe un contexto. Lo usan voraz y backtracking para ordenar las materias
// elegibles segun su prioridad. No es estable: el comparador debe resolver
// los empates (los de voraz/backtracking terminan en el indice).
// Pivote: elemento central (evita el peor caso con entradas ya ordenadas).

static void quicksort_indices_rec(int *idx, int inicio, int fin,
                                  ComparadorIdx cmp, const void *ctx) {
    while (inicio < fin) {
        int medio = inicio + (fin - inicio) / 2;
        int t = idx[medio]; idx[medio] = idx[fin]; idx[fin] = t;   // pivote al final
        int pivote = idx[fin];

        int i = inicio - 1;
        for (int j = inicio; j < fin; j++) {
            if (cmp(idx[j], pivote, ctx) <= 0) {
                i++;
                t = idx[i]; idx[i] = idx[j]; idx[j] = t;
            }
        }
        t = idx[i + 1]; idx[i + 1] = idx[fin]; idx[fin] = t;
        int p = i + 1;

        // recursion en la mitad mas chica, iteracion en la otra (pila O(log n))
        if (p - inicio < fin - p) {
            quicksort_indices_rec(idx, inicio, p - 1, cmp, ctx);
            inicio = p + 1;
        } else {
            quicksort_indices_rec(idx, p + 1, fin, cmp, ctx);
            fin = p - 1;
        }
    }
}

void quicksort_indices(int *idx, int n, ComparadorIdx cmp, const void *ctx) {
    if (n > 1) {
        quicksort_indices_rec(idx, 0, n - 1, cmp, ctx);
    }
}
