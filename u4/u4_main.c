               [-n limite_pasos] [-t] [--sin-plan]
#include "validacion.h"
#include "elegibles.h"
#include "backtracking.h"
#include "leerArchivo.h"
#include <time.h>


int main(int argc, char *argv[]) {
    const char *ruta = "../quicksort/cursos.txt";
    const char *hist = NULL;
    int sem_aprob = -1, max_cred = 50, max_mat = 0, ver_topo = 0, sin_plan = 0;
    long long limite = 20000000LL;

    
    int n = 0;
    Curso *cursos = leer_cursos_archivo(ruta, &n);
    if (cursos == NULL || n == 0) {
        printf("No se pudieron leer cursos desde '%s'.\n", ruta);
        return 1;
    }
    printf("Malla: %s | %d materias\n", ruta, n);

    // ---------- 1) Validacion de la malla ----------
    printf("\n== 1) Validacion de la malla [EXACTO: DFS, O(V+E)] ==\n");
    int malos = validar_indices(cursos, n);
    Ciclo ciclos[MAX_CICLOS];
    int nc = detectar_ciclos(cursos, n, ciclos, MAX_CICLOS);
    if (nc > 0) {
        printf("  [ERROR] La malla tiene ciclos de prerrequisitos (%d arista%s de retorno):\n",
               nc, nc == 1 ? "" : "s");
        for (int i = 0; i < nc && i < MAX_CICLOS; i++) imprimir_ciclo(cursos, &ciclos[i]);
        printf("  Corrige la malla antes de planear.\n");
        liberar_arreglo_cursos(cursos, n);
        return 2;
    }
    if (malos > 0) {
        printf("  [ERROR] Hay %d prerrequisito(s) invalido(s). Corrige la malla.\n", malos);
        liberar_arreglo_cursos(cursos, n);
        return 2;
    }
    printf("  OK: sin ciclos ni indices invalidos.\n");

    int *orden = (int *)malloc((size_t)n * sizeof(int));
    if (orden != NULL && orden_topologico(cursos, n, orden) == 0) {
        int mostrar = ver_topo ? n : (n < 8 ? n : 8);
        printf("  Orden topologico (un orden valido, no unico)%s:\n", ver_topo ? "" : " primeras materias");
        for (int i = 0; i < mostrar; i++) printf("    %2d. %s\n", i + 1, cursos[orden[i]].nombre);
        if (mostrar < n) printf("    ... (usa -t para verlo completo)\n");
    }
    free(orden);

    // ---------- 2) Historial y materias elegibles ----------
    char *aprobadas = (char *)calloc((size_t)n, 1);
    if (hist != NULL) {
        if (parsear_historial(hist, n, aprobadas) < 0) { free(aprobadas); liberar_arreglo_cursos(cursos, n); return 1; }
    } else if (sem_aprob >= 0) {
        aprobar_hasta_semestre(cursos, n, sem_aprob, aprobadas);
    }
    int num_aprob = 0, cred_aprob = 0, cred_total = 0;
    for (int i = 0; i < n; i++) {
        cred_total += cursos[i].creditos;
        if (aprobadas[i]) { num_aprob++; cred_aprob += cursos[i].creditos; }
    }
    printf("\n== 2) Materias elegibles [EXACTO: O(V+E)] ==\n");
    printf("  Aprobadas: %d de %d materias (%d de %d creditos)\n", num_aprob, n, cred_aprob, cred_total);
    validar_historial(cursos, n, aprobadas);

    int *elegibles = (int *)malloc((size_t)n * sizeof(int));
    int ne = materias_elegibles(cursos, n, aprobadas, elegibles);
    printf("  Elegibles el siguiente periodo: %d\n", ne);
    for (int k = 0; k < ne; k++) {
        printf("    - [%2d] %-50s cr:%d sem.plan:%d\n", elegibles[k], cursos[elegibles[k]].nombre,
               cursos[elegibles[k]].creditos, cursos[elegibles[k]].semestre);
    }
    free(elegibles);

    if (!sin_plan) {
        printf("\n== 3) Planeacion por backtracking (tope: %d creditos, %d materias por semestre) ==\n",
               max_cred, max_mat);
        ResultadoPlan res;
        clock_t t0 = clock();
        int r = plan_backtracking(cursos, n, aprobadas, max_cred, max_mat, limite, &res);
        double ms = 1000.0 * (double)(clock() - t0) / CLOCKS_PER_SEC;
        if (r == -2) printf("  Instancia demasiado grande (n > 64).\n");
        else if (r == -1) printf("  Error: ciclo de prerrequisitos o memoria insuficiente.\n");
        else {
            imprimir_plan_backtracking(cursos, n, &res);
            printf("Tiempo: %.1f ms\n", ms);
        }
        liberar_resultado_plan(&res);
    }

    free(aprobadas);
    liberar_arreglo_cursos(cursos, n);
    return 0;
}
