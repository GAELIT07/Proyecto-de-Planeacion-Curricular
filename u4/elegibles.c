#include "elegibles.h"

int parsear_historial(const char *texto, int n, char *aprobadas) {
    memset(aprobadas, 0, (size_t)n);
    if (texto == NULL || texto[0] == '\0') {
        return 0;
    }
    char copia[1024];
    strncpy(copia, texto, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    int marcadas = 0;
    for (char *tok = strtok(copia, ","); tok != NULL; tok = strtok(NULL, ",")) {
        int a, b;
        if (sscanf(tok, " %d - %d", &a, &b) == 2) {
            // rango a-b
        } else if (sscanf(tok, " %d", &a) == 1) {
            b = a;
        } else {
            printf("Historial invalido: '%s'\n", tok);
            return -1;
        }
        if (a < 0 || b >= n || a > b) {
            printf("Historial invalido: el rango %d-%d esta fuera de 0..%d\n", a, b, n - 1);
            return -1;
        }
        for (int i = a; i <= b; i++) {
            if (!aprobadas[i]) {
                aprobadas[i] = 1;
                marcadas++;
            }
        }
    }
    return marcadas;
}

void aprobar_hasta_semestre(const Curso *cursos, int n, int sem, char *aprobadas) {
    for (int i = 0; i < n; i++) {
        aprobadas[i] = (cursos[i].semestre <= sem) ? 1 : 0;
    }
}

int validar_historial(const Curso *cursos, int n, const char *aprobadas) {
    int malas = 0;
    for (int i = 0; i < n; i++) {
        if (!aprobadas[i]) {
            continue;
        }
        for (int j = 0; j < cursos[i].num_prerequisitos; j++) {
            int p = cursos[i].prerequisitos[j];
            if (p >= 0 && p < n && !aprobadas[p]) {
                printf("  [AVISO] '%s' esta aprobada pero su prerrequisito '%s' no\n",
                       cursos[i].nombre, cursos[p].nombre);
                malas++;
            }
        }
    }
    return malas;
}

int materias_elegibles(const Curso *cursos, int n, const char *aprobadas, int *salida) {
    int cuenta = 0;
    for (int i = 0; i < n; i++) {
        if (aprobadas[i]) {
            continue;
        }
        int libre = 1;
        for (int j = 0; j < cursos[i].num_prerequisitos && libre; j++) {
            int p = cursos[i].prerequisitos[j];
            if (p < 0 || p >= n) {
                continue;   // indice invalido
            }
            if (p == i || !aprobadas[p]) {
                libre = 0;
            }
        }
        if (libre) {
            salida[cuenta++] = i;
        }
    }
    return cuenta;
}
