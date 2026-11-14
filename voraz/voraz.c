#include "voraz.h"
#include "quicksort.h"

static int prereq_valido(int i, int p, int n) {
    return p >= 0 && p < n && p != i;
}

// DFS sobre los hijos (materias que dependen de i) para la ruta critica.
// estado: 0 = sin visitar, 1 = en curso, 2 = terminado. Devuelve -1 si hay ciclo.
static int dfs_ruta(int i, int n, const int *ini, const int *hijos,
                    int *estado, int *ruta) {
    estado[i] = 1;
    int mejor = 0;
    for (int k = ini[i]; k < ini[i + 1]; k++) {
        int h = hijos[k];
        if (estado[h] == 1) {
            return -1;
        }
        if (estado[h] == 0 && dfs_ruta(h, n, ini, hijos, estado, ruta) != 0) {
            return -1;
        }
        if (ruta[h] > mejor) {
            mejor = ruta[h];
        }
    }
    ruta[i] = 1 + mejor;
    estado[i] = 2;
    return 0;
}

int calcular_metricas_voraz(const Curso *cursos, int n, MetricasVoraz *m) {
    m->desbloquea = NULL;
    m->ruta_critica = NULL;
    if (n <= 0) {
        return 0;
    }

    // Lista de adyacencia de hijos (prereq -> curso que lo requiere), formato CSR
    int *ini = (int *)calloc((size_t)n + 1, sizeof(int));
    int total = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < cursos[i].num_prerequisitos; j++) {
            int p = cursos[i].prerequisitos[j];
            if (prereq_valido(i, p, n)) {
                ini[p + 1]++;
                total++;
            }
        }
    }
    for (int i = 0; i < n; i++) {
        ini[i + 1] += ini[i];
    }
    int *hijos = (int *)malloc((size_t)(total > 0 ? total : 1) * sizeof(int));
    int *pos = (int *)malloc((size_t)n * sizeof(int));
    int *estado = (int *)calloc((size_t)n, sizeof(int));
    int *visto = (int *)malloc((size_t)n * sizeof(int));
    int *cola = (int *)malloc((size_t)n * sizeof(int));
    m->desbloquea = (int *)calloc((size_t)n, sizeof(int));
    m->ruta_critica = (int *)calloc((size_t)n, sizeof(int));

    int ok = ini && hijos && pos && estado && visto && cola &&
             m->desbloquea && m->ruta_critica;
    if (ok) {
        memcpy(pos, ini, (size_t)n * sizeof(int));
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < cursos[i].num_prerequisitos; j++) {
                int p = cursos[i].prerequisitos[j];
                if (prereq_valido(i, p, n)) {
                    hijos[pos[p]++] = i;
                }
            }
        }

        // Ruta critica (y deteccion de ciclos)
        for (int i = 0; i < n && ok; i++) {
            if (estado[i] == 0 && dfs_ruta(i, n, ini, hijos, estado, m->ruta_critica) != 0) {
                ok = 0;
            }
        }

        // Descendientes transitivos: BFS desde cada materia
        for (int i = 0; i < n && ok; i++) {
            memset(visto, 0, (size_t)n * sizeof(int));
            int cab = 0, fin = 0, cuenta = 0;
            cola[fin++] = i;
            visto[i] = 1;
            while (cab < fin) {
                int u = cola[cab++];
                for (int k = ini[u]; k < ini[u + 1]; k++) {
                    int h = hijos[k];
                    if (!visto[h]) {
                        visto[h] = 1;
                        cola[fin++] = h;
                        cuenta++;
                    }
                }
            }
            m->desbloquea[i] = cuenta;
        }
    }

    free(ini);
    free(hijos);
    free(pos);
    free(estado);
    free(visto);
    free(cola);
    if (!ok) {
        liberar_metricas_voraz(m);
        return -1;
    }
    return 0;
}

void liberar_metricas_voraz(MetricasVoraz *m) {
    free(m->desbloquea);
    free(m->ruta_critica);
    m->desbloquea = NULL;
    m->ruta_critica = NULL;
}

// Devuelve > 0 si la materia a tiene MAS prioridad que b (debe ir antes).
static int prioridad(const Curso *cursos, const MetricasVoraz *m,
                     CriterioVoraz criterio, int a, int b) {
    if (criterio == VORAZ_DESBLOQUEA) {
        if (m->desbloquea[a] != m->desbloquea[b]) {
            return m->desbloquea[a] - m->desbloquea[b];
        }
        if (m->ruta_critica[a] != m->ruta_critica[b]) {
            return m->ruta_critica[a] - m->ruta_critica[b];
        }
    }
    if (cursos[a].creditos != cursos[b].creditos) {
        return cursos[a].creditos - cursos[b].creditos;
    }
    return b - a;   // menor indice primero
}

typedef struct {
    const Curso *cursos;
    const MetricasVoraz *m;
    CriterioVoraz criterio;
} CtxPrioridad;

// Adaptador para quicksort_indices: mayor prioridad = va primero.
static int comparar_prioridad(int a, int b, const void *ctx) {
    const CtxPrioridad *x = (const CtxPrioridad *)ctx;
    return -prioridad(x->cursos, x->m, x->criterio, a, b);
}

int seleccion_voraz(const Curso *cursos, int n, int max_creditos,
                    int max_materias, CriterioVoraz criterio, int *asignacion) {
    if (n <= 0) {
        return 0;
    }
    MetricasVoraz m;
    if (calcular_metricas_voraz(cursos, n, &m) != 0) {
        return -1;
    }

    int *elegibles = (int *)malloc((size_t)n * sizeof(int));
    if (elegibles == NULL) {
        liberar_metricas_voraz(&m);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        asignacion[i] = 0;   // 0 = aun sin asignar
    }

    int pendientes = n;
    int semestre = 0;
    while (pendientes > 0) {
        semestre++;

        // 1) Materias elegibles: todos sus prerrequisitos en semestres anteriores
        int ne = 0;
        for (int i = 0; i < n; i++) {
            if (asignacion[i] != 0) {
                continue;
            }
            int libre = 1;
            for (int j = 0; j < cursos[i].num_prerequisitos && libre; j++) {
                int p = cursos[i].prerequisitos[j];
                if (prereq_valido(i, p, n) && (asignacion[p] == 0 || asignacion[p] >= semestre)) {
                    libre = 0;
                }
            }
            if (libre) {
                elegibles[ne++] = i;
            }
        }

        // 2) Ordenar por prioridad con quicksort 
        CtxPrioridad ctx = { cursos, &m, criterio };
        quicksort_indices(elegibles, ne, comparar_prioridad, &ctx);

        // 3) Tomar en orden mientras quepan. Si una no cabe, se prueba con
        //    la siguiente (la que no cabe espera al siguiente semestre).
        int creditos = 0, tomadas = 0;
        for (int k = 0; k < ne; k++) {
            int c = elegibles[k];
            if (max_materias > 0 && tomadas >= max_materias) {
                break;
            }
            if (max_creditos > 0 && tomadas > 0 && creditos + cursos[c].creditos > max_creditos) {
                continue;
            }
            asignacion[c] = semestre;
            creditos += cursos[c].creditos;
            tomadas++;
            pendientes--;
        }

        if (tomadas == 0) {   // solo ocurre con ciclos; evita ciclo infinito
            free(elegibles);
            liberar_metricas_voraz(&m);
            return -1;
        }
    }

    free(elegibles);
    liberar_metricas_voraz(&m);
    return semestre;
}

void imprimir_plan_voraz(const Curso *cursos, int n, const int *asignacion,
                         int num_semestres, const MetricasVoraz *m,
                         const char *titulo) {
    printf("\n============================================================\n");
    printf("%s\n", titulo);
    printf("============================================================\n");

    for (int s = 1; s <= num_semestres; s++) {
        int cant = 0, creditos = 0;
        for (int i = 0; i < n; i++) {
            if (asignacion[i] == s) {
                cant++;
                creditos += cursos[i].creditos;
            }
        }
        if (cant == 0) {
            continue;
        }
        printf("\nSemestre %d | %d materias | %d créditos\n", s, cant, creditos);
        for (int i = 0; i < n; i++) {
            if (asignacion[i] == s) {
                printf("  - %-40s cr:%d  desbloquea:%d  ruta:%d\n",
                       cursos[i].nombre, cursos[i].creditos,
                       m ? m->desbloquea[i] : 0, m ? m->ruta_critica[i] : 0);
            }
        }
    }
    printf("\nTotal: %d semestres\n", num_semestres);
}
