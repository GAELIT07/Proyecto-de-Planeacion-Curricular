#include "validacion.h"

typedef struct {
    const Curso *c;
    int n;
    int *estado;
    int *pila;
    int prof;
    Ciclo *salida;
    int max;
    int total;
} CtxCiclo;

int validar_indices(const Curso *cursos, int n) {
    int malos = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < cursos[i].num_prerequisitos; j++) {
            int p = cursos[i].prerequisitos[j];
            if (p < 0 || p >= n) {
                printf("  [ERROR] '%s' (indice %d) tiene un prerrequisito fuera de rango: %d\n",
                       cursos[i].nombre, i, p);
                malos++;
            }
        }
    }
    return malos;
}

static void dfs_ciclo(CtxCiclo *x, int u) {
    x->estado[u] = 1;
    x->pila[x->prof++] = u;

    for (int j = 0; j < x->c[u].num_prerequisitos; j++) {
        int p = x->c[u].prerequisitos[j];
        if (p < 0 || p >= x->n) {
            continue;   // indice invalido: lo reporta validar_indices
        }
        if (x->estado[p] == 1) {
            // Arista de retorno: el ciclo es el tramo de la pila desde p hasta u
            if (x->total < x->max) {
                int k = x->prof - 1;
                while (x->pila[k] != p) {
                    k--;
                }
                Ciclo *cy = &x->salida[x->total];
                cy->longitud = 0;
                for (; k < x->prof && cy->longitud < MAX_LONG_CICLO; k++) {
                    cy->nodos[cy->longitud++] = x->pila[k];
                }
            }
            x->total++;
        } else if (x->estado[p] == 0) {
            dfs_ciclo(x, p);
        }
    }

    x->prof--;
    x->estado[u] = 2;
}

int detectar_ciclos(const Curso *cursos, int n, Ciclo *ciclos, int max_ciclos) {
    if (n <= 0) {
        return 0;
    }
    CtxCiclo x;
    x.c = cursos;
    x.n = n;
    x.estado = (int *)calloc((size_t)n, sizeof(int));
    x.pila = (int *)malloc((size_t)n * sizeof(int));
    x.prof = 0;
    x.salida = ciclos;
    x.max = max_ciclos;
    x.total = 0;
    if (x.estado == NULL || x.pila == NULL) {
        free(x.estado);
        free(x.pila);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        if (x.estado[i] == 0) {
            dfs_ciclo(&x, i);
        }
    }
    free(x.estado);
    free(x.pila);
    return x.total;
}

void imprimir_ciclo(const Curso *cursos, const Ciclo *ciclo) {
    printf("  Ciclo de %d materia%s: ", ciclo->longitud, ciclo->longitud == 1 ? "" : "s");
    for (int k = 0; k < ciclo->longitud; k++) {
        printf("%s -> ", cursos[ciclo->nodos[k]].nombre);
    }
    printf("%s\n", cursos[ciclo->nodos[0]].nombre);
    printf("    (cada flecha se lee \"requiere a\")\n");
}

static int dfs_topo(const Curso *c, int n, int u, int *estado, int *orden, int *cnt) {
    estado[u] = 1;
    for (int j = 0; j < c[u].num_prerequisitos; j++) {
        int p = c[u].prerequisitos[j];
        if (p < 0 || p >= n) {
            continue;
        }
        if (estado[p] == 1) {
            return -1;
        }
        if (estado[p] == 0 && dfs_topo(c, n, p, estado, orden, cnt) != 0) {
            return -1;
        }
    }
    estado[u] = 2;
    orden[(*cnt)++] = u;   // postorden: primero salen sus prerrequisitos
    return 0;
}

int orden_topologico(const Curso *cursos, int n, int *orden) {
    int *estado = (int *)calloc((size_t)(n > 0 ? n : 1), sizeof(int));
    int cnt = 0;
    int r = 0;
    if (estado == NULL) {
        return -1;
    }
    for (int i = 0; i < n && r == 0; i++) {
        if (estado[i] == 0) {
            r = dfs_topo(cursos, n, i, estado, orden, &cnt);
        }
    }
    free(estado);
    return r;
}
