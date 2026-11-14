// Demo del modulo voraz: compara el criterio "desbloquea" contra "creditos".
// Uso: ./voraz <cursos.txt> [max_creditos] [max_materias]
#include "voraz.h"
#include "leerArchivo.h"

int main(int argc, char *argv[]) {
    const char *ruta = (argc > 1) ? argv[1] : "../quicksort/cursos.txt";
    int max_cred = (argc > 2) ? atoi(argv[2]) : 50;
    int max_mat = (argc > 3) ? atoi(argv[3]) : 0;

    int n = 0;

    Curso *cursos = leer_cursos_archivo(ruta, &n);

    if (cursos == NULL || n == 0) {
        printf("No se pudieron leer cursos desde '%s'.\n", ruta);
        return 1;
    }
    printf("Cursos: %d | tope: %d créditos, %d materias por semestre\n",
           n, max_cred, max_mat);

    MetricasVoraz m;
    if (calcular_metricas_voraz(cursos, n, &m) != 0) {
        printf("Hay un ciclo de prerrequisitos.\n");
        liberar_arreglo_cursos(cursos, n);
        return 1;
    }

    int *plan_a = (int *)malloc((size_t)n * sizeof(int));
    int *plan_b = (int *)malloc((size_t)n * sizeof(int));
    int sa = seleccion_voraz(cursos, n, max_cred, max_mat, VORAZ_DESBLOQUEA, plan_a);
    int sb = seleccion_voraz(cursos, n, max_cred, max_mat, VORAZ_CREDITOS, plan_b);

    imprimir_plan_voraz(cursos, n, plan_a, sa, &m, "Plan voraz: prioridad por materias que desbloquea");
    printf("\nComparacion: desbloquea = %d semestres | solo créditos = %d semestres\n", sa, sb);

    free(plan_a);
    free(plan_b);
    liberar_metricas_voraz(&m);
    liberar_arreglo_cursos(cursos, n);
    return 0;
}
