// Programa de prueba del modulo de ordenamiento (U2).
// Se compila junto con quicksort.c; quedo separado para que voraz y u4
// puedan usar quicksort.c sin chocar con otro main().
#include "quicksort.h"

int main(int argc, char *argv[]) {
    const char *ruta_archivo = (argc > 1) ? argv[1] : "cursos.txt";

    int n = 0;
    Curso *cursos = leer_cursos_archivo(ruta_archivo, &n);

    if (cursos == NULL || n == 0) {
        printf("No se pudieron leer cursos desde '%s'.\n", ruta_archivo);
        printf("Verifica que el archivo exista y este en el mismo directorio, ");
        printf("o pasa la ruta como argumento: %s <ruta_al_archivo>\n",
               (argc > 0) ? argv[0] : "programa");
        return 1;
    }

    printf("Se leyeron %d cursos desde '%s'\n", n, ruta_archivo);

    // Mostrar datos originales tal como se leyeron del archivo
    imprimir_cursos(cursos, n, "Lista Original (leida desde archivo)", cursos);

    // Prueba Quicksort por dificultad
    Curso *copia_qs = copiar_arreglo_cursos(cursos, n);
    quicksort(copia_qs, n, comparar_dificultad);
    imprimir_cursos(copia_qs, n, "Ordenado con Quicksort (por Dificultad)", cursos);
    liberar_arreglo_cursos(copia_qs, n);

    // Prueba Quicksort por semestre
    Curso *copia_ms = copiar_arreglo_cursos(cursos, n);
    quicksort(copia_ms, n, comparar_semestre);
    imprimir_cursos(copia_ms, n, "Ordenado con Quicksort (por Semestre)", cursos);
    liberar_arreglo_cursos(copia_ms, n);

    // Prueba Quicksort por créditos
    Curso *copia_qs_cred = copiar_arreglo_cursos(cursos, n);
    quicksort(copia_qs_cred, n, comparar_creditos);
    imprimir_cursos(copia_qs_cred, n, "Ordenado con Quicksort (por Créditos)", cursos);
    liberar_arreglo_cursos(copia_qs_cred, n);

    // Prueba Quicksort por nombre
    Curso *copia_qs_nombre = copiar_arreglo_cursos(cursos, n);
    quicksort(copia_qs_nombre, n, comparar_nombre);
    imprimir_cursos(copia_qs_nombre, n, "Ordenado con Quicksort (por Nombre)", cursos);
    liberar_arreglo_cursos(copia_qs_nombre, n);

    // Prueba Quicksort con prioridad a las materias reprobadas.
    // Uso: planeacion cursos.txt 8 9 20   (los numeros son indices base 0
    // de las materias reprobadas, en el orden del archivo)
    if (argc > 2) {
        char *reprobadas = (char *)calloc((size_t)n, 1);
        for (int k = 2; k < argc; k++) {
            int idx = atoi(argv[k]);
            if (reprobadas != NULL && idx >= 0 && idx < n) {
                reprobadas[idx] = 1;
            }
        }

        Curso *copia_rep = copiar_arreglo_cursos(cursos, n);
        for (int i = 0; i < n; i++) {
            copia_rep[i].indice = i;   // la prioridad se consulta por indice
        }

        Criterio criterio = { CRIT_DIFICULTAD, 1 };   // dificultad, de mayor a menor
        configurar_criterios(&criterio, 1);
        configurar_reprobadas(reprobadas, n);
        quicksort(copia_rep, n, comparar_con_prioridad);
        configurar_reprobadas(NULL, 0);

        imprimir_cursos(copia_rep, n,
                        "Ordenado con Quicksort (reprobadas primero, luego por Dificultad de mayor a menor)",
                        cursos);
        liberar_arreglo_cursos(copia_rep, n);
        free(reprobadas);
    }

    // Liberar memoria original
    liberar_arreglo_cursos(cursos, n);

    return 0;
}
