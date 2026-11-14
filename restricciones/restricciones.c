#include "restricciones.h"

int calcular_niveles(Curso *cursos, int n) {
    for (int i = 0; i < n; i++) {
        cursos[i].indice = i;
        cursos[i].nivel = 0;
    }

    // Se repite hasta que ningun nivel cambie. Sin ciclos, esto termina en
    // a lo mas n pasadas; si sigue cambiando despues de eso, hay un ciclo.
    for (int pasada = 0; pasada <= n; pasada++) {
        int hubo_cambio = 0;

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < cursos[i].num_prerequisitos; j++) {
                int p = cursos[i].prerequisitos[j];
                if (p < 0 || p >= n || p == i) {
                    continue;
                }
                if (cursos[i].nivel < cursos[p].nivel + 1) {
                    cursos[i].nivel = cursos[p].nivel + 1;
                    hubo_cambio = 1;
                }
            }
        }

        if (!hubo_cambio) {
            return 0;
        }
    }
    return -1;
}

int comparar_restricciones(const void *a, const void *b) {
    const Curso *c1 = (const Curso *)a;
    const Curso *c2 = (const Curso *)b;

    if (c1->nivel != c2->nivel) {
        return c1->nivel - c2->nivel;
    }
    if (c1->semestre != c2->semestre) {
        return c1->semestre - c2->semestre;
    }
    return strcmp(c1->nombre, c2->nombre);
}
