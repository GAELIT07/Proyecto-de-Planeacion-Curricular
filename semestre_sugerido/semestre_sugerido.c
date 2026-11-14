#include "semestre_sugerido.h"
#include "../quicksort/quicksort.h"
#include "../restricciones/restricciones.h"

int semestre_sugerido(Curso *cursos, int n, int max_creditos,
                      int max_dificultad, int *asignacion) {
    if (n <= 0) {
        return 0;
    }
    if (calcular_niveles(cursos, n) != 0) {
        return -1;
    }

    int semestre_max_plan = 0;
    for (int i = 0; i < n; i++) {
        if (cursos[i].semestre > semestre_max_plan) {
            semestre_max_plan = cursos[i].semestre;
        }
        asignacion[i] = 0;
    }

    // Un curso puede retrasarse a lo mas n semestres despues del plan.
    int tam = semestre_max_plan + n + 2;
    int *cantidad = (int *)calloc((size_t)tam, sizeof(int));
    int *creditos = (int *)calloc((size_t)tam, sizeof(int));
    int *dificultad = (int *)calloc((size_t)tam, sizeof(int));
    Curso *ordenados = copiar_arreglo_cursos(cursos, n);

    if (cantidad == NULL || creditos == NULL || dificultad == NULL || ordenados == NULL) {
        free(cantidad);
        free(creditos);
        free(dificultad);
        if (ordenados != NULL) {
            liberar_arreglo_cursos(ordenados, n);
        }
        return -1;
    }

    // Con este orden, los prerrequisitos de un curso ya tienen semestre
    // asignado cuando le toca (siempre tienen un nivel menor).
    quicksort(ordenados, n, comparar_restricciones);

    int usados = 0;
    for (int k = 0; k < n; k++) {
        const Curso *c = &ordenados[k];

        // Semestre minimo: el del plan, y despues de todos sus prerrequisitos
        int sem = c->semestre > 1 ? c->semestre : 1;
        for (int j = 0; j < c->num_prerequisitos; j++) {
            int p = c->prerequisitos[j];
            if (p >= 0 && p < n && p != c->indice && asignacion[p] + 1 > sem) {
                sem = asignacion[p] + 1;
            }
        }

        // Si el semestre ya esta lleno, pasa al siguiente. En un semestre
        // vacio siempre cabe, aunque el curso solo pase el tope.
        while (cantidad[sem] > 0 &&
               ((max_creditos > 0 && creditos[sem] + c->creditos > max_creditos) ||
                (max_dificultad > 0 && dificultad[sem] + c->dificultad > max_dificultad))) {
            sem++;
        }

        asignacion[c->indice] = sem;
        cantidad[sem]++;
        creditos[sem] += c->creditos;
        dificultad[sem] += c->dificultad;
        if (sem > usados) {
            usados = sem;
        }
    }

    liberar_arreglo_cursos(ordenados, n);
    free(cantidad);
    free(creditos);
    free(dificultad);
    return usados;
}

void imprimir_plan(const Curso *cursos, int n, const int *asignacion,
                   int num_semestres) {
    printf("\n");
    printf("============================================================\n");
    printf("Plan sugerido por semestre\n");
    printf("============================================================\n");

    for (int s = 1; s <= num_semestres; s++) {
        int cantidad = 0, creditos = 0, dificultad = 0;
        for (int i = 0; i < n; i++) {
            if (asignacion[i] == s) {
                cantidad++;
                creditos += cursos[i].creditos;
                dificultad += cursos[i].dificultad;
            }
        }
        if (cantidad == 0) {
            continue;
        }

        printf("\nSemestre %d | %d materias | %d créditos | dificultad total %d\n",
               s, cantidad, creditos, dificultad);
        for (int i = 0; i < n; i++) {
            if (asignacion[i] == s) {
                printf("  %c %s (plan: semestre %d)\n",
                       cursos[i].semestre != s ? '*' : '-',
                       cursos[i].nombre, cursos[i].semestre);
            }
        }
    }
    printf("\n");
}
