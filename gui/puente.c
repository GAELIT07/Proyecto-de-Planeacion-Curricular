/*
 * puente.c - Puente entre la interfaz en Python (ctypes) y los modulos en C.
 *
 * Mantiene la malla cargada en memoria y expone funciones con tipos simples
 * (int, char*, arreglos de int) para que main.py las llame:
 *
 *   - carga / consulta de cursos      (leerArchivo + curso)
 *   - ordenamiento                    (quicksort, con prioridad a reprobadas)
 *   - materias elegibles              (u4/elegibles)
 *   - validacion de la malla          (u4/validacion: DFS, ciclos, topologico)
 *   - semestre sugerido               (semestre_sugerido + restricciones)
 *   - seleccion voraz                 (voraz)
 *   - planeacion por backtracking     (u4/backtracking)
 *
 * No modifica ningun modulo existente: solo los usa.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curso.h"
#include "leerArchivo.h"
#include "quicksort.h"
#include "restricciones.h"
#include "semestre_sugerido.h"
#include "voraz.h"
#include "validacion.h"
#include "elegibles.h"
#include "backtracking.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

/* ------------------------------------------------------------------ */
/* Malla cargada                                                       */
/* ------------------------------------------------------------------ */

static Curso *g_cursos = NULL;
static int g_n = 0;

static Ciclo g_ciclos[MAX_CICLOS];
static int g_num_ciclos = 0;

static int indice_valido(int i) {
    return g_cursos != NULL && i >= 0 && i < g_n;
}

/* Lee el archivo y deja los cursos en memoria. Devuelve cuantos leyo. */
EXPORT int cargar_cursos(const char *ruta) {
    if (g_cursos != NULL) {
        liberar_arreglo_cursos(g_cursos, g_n);
        g_cursos = NULL;
        g_n = 0;
    }
    int n = 0;
    Curso *leidos = leer_cursos_archivo(ruta, &n);
    if (leidos == NULL || n <= 0) {
        if (leidos != NULL) {
            liberar_arreglo_cursos(leidos, n);
        }
        return 0;
    }
    g_cursos = leidos;
    g_n = n;
    return g_n;
}

EXPORT int obtener_total_cursos(void) {
    return g_n;
}

EXPORT const char *obtener_nombre(int i) {
    return indice_valido(i) ? g_cursos[i].nombre : "";
}

EXPORT int obtener_creditos(int i) {
    return indice_valido(i) ? g_cursos[i].creditos : 0;
}

EXPORT int obtener_semestre(int i) {
    return indice_valido(i) ? g_cursos[i].semestre : 0;
}

EXPORT int obtener_dificultad(int i) {
    return indice_valido(i) ? g_cursos[i].dificultad : 0;
}

EXPORT int obtener_num_prerequisitos(int i) {
    return indice_valido(i) ? g_cursos[i].num_prerequisitos : 0;
}

EXPORT int obtener_prerequisito(int i, int j) {
    if (!indice_valido(i) || j < 0 || j >= g_cursos[i].num_prerequisitos) {
        return -1;
    }
    return g_cursos[i].prerequisitos[j];
}

EXPORT void liberar_cursos(void) {
    if (g_cursos != NULL) {
        liberar_arreglo_cursos(g_cursos, g_n);
    }
    g_cursos = NULL;
    g_n = 0;
}

/* ------------------------------------------------------------------ */
/* Ordenamiento con quicksort                                          */
/* ------------------------------------------------------------------ */

/*
 * Ordena una COPIA de la malla con quicksort y llena salida[] con los
 * indices originales en el orden resultante.
 *   campo: 0 = creditos, 1 = dificultad, 2 = semestre, 3 = nombre
 * Los empates se desempatan por nombre (comparar_compuesto).
 * Devuelve cuantos indices escribio, o -1 si el criterio no existe.
 */
EXPORT int ordenar_cursos(int campo, int descendente, int *salida) {
    if (g_cursos == NULL || campo < 0 || campo > 3) {
        return -1;
    }
    for (int i = 0; i < g_n; i++) {
        g_cursos[i].indice = i;
    }
    Curso *copia = copiar_arreglo_cursos(g_cursos, g_n);
    if (copia == NULL) {
        return -1;
    }

    Criterio criterio;
    criterio.campo = (CampoCriterio)campo;
    criterio.descendente = descendente ? 1 : 0;
    configurar_criterios(&criterio, 1);

    quicksort(copia, g_n, comparar_compuesto);

    for (int k = 0; k < g_n; k++) {
        salida[k] = copia[k].indice;
    }
    liberar_arreglo_cursos(copia, g_n);
    return g_n;
}

/*
 * Igual que ordenar_cursos, pero las materias REPROBADAS van primero
 * (quicksort con comparar_con_prioridad); dentro de cada grupo se respeta
 * el criterio elegido.
 *   reprobadas: g_n bytes, reprobadas[i] = 1 si la materia i esta reprobada.
 * Devuelve cuantos indices escribio, o -1 si hay error.
 */
EXPORT int ordenar_cursos_prioridad(int campo, int descendente,
                                    const char *reprobadas, int *salida) {
    if (g_cursos == NULL || reprobadas == NULL || campo < 0 || campo > 3) {
        return -1;
    }
    for (int i = 0; i < g_n; i++) {
        g_cursos[i].indice = i;
    }
    Curso *copia = copiar_arreglo_cursos(g_cursos, g_n);
    if (copia == NULL) {
        return -1;
    }

    Criterio criterio;
    criterio.campo = (CampoCriterio)campo;
    criterio.descendente = descendente ? 1 : 0;
    configurar_criterios(&criterio, 1);
    configurar_reprobadas(reprobadas, g_n);

    quicksort(copia, g_n, comparar_con_prioridad);

    configurar_reprobadas(NULL, 0);

    for (int k = 0; k < g_n; k++) {
        salida[k] = copia[k].indice;
    }
    liberar_arreglo_cursos(copia, g_n);
    return g_n;
}

/* ------------------------------------------------------------------ */
/* Materias elegibles                                                  */
/* ------------------------------------------------------------------ */

/* aprobadas: n bytes (1 = aprobada). Llena salida[] con indices elegibles. */
EXPORT int calcular_elegibles(const char *aprobadas, int *salida) {
    if (g_cursos == NULL) {
        return 0;
    }
    return materias_elegibles(g_cursos, g_n, aprobadas, salida);
}

/* ------------------------------------------------------------------ */
/* Validacion de la malla                                              */
/* ------------------------------------------------------------------ */

/* Cuenta los prerrequisitos con indice fuera de rango. */
EXPORT int validar_indices_malla(void) {
    if (g_cursos == NULL) {
        return 0;
    }
    return validar_indices(g_cursos, g_n);
}

/* Busca ciclos y los guarda. Devuelve cuantos se encontraron (-1 = error). */
EXPORT int buscar_ciclos(void) {
    g_num_ciclos = 0;
    if (g_cursos == NULL) {
        return 0;
    }
    int total = detectar_ciclos(g_cursos, g_n, g_ciclos, MAX_CICLOS);
    if (total < 0) {
        return -1;
    }
    g_num_ciclos = total < MAX_CICLOS ? total : MAX_CICLOS;
    return total;
}

/* Copia el ciclo k (indices de materias) en nodos[]; devuelve su longitud. */
EXPORT int obtener_ciclo(int k, int *nodos, int max) {
    if (k < 0 || k >= g_num_ciclos) {
        return 0;
    }
    int len = g_ciclos[k].longitud < max ? g_ciclos[k].longitud : max;
    for (int i = 0; i < len; i++) {
        nodos[i] = g_ciclos[k].nodos[i];
    }
    return len;
}

/* Orden topologico en salida[]. 0 si todo bien, -1 si hay ciclo. */
EXPORT int orden_topologico_malla(int *salida) {
    if (g_cursos == NULL) {
        return -1;
    }
    return orden_topologico(g_cursos, g_n, salida);
}

/* ------------------------------------------------------------------ */
/* Semestre sugerido (quicksort + restricciones)                       */
/* ------------------------------------------------------------------ */

/*
 * Llena asignacion[i] con el semestre sugerido del curso i.
 * Devuelve el numero de semestres usados, o -1 si hay ciclo.
 */
EXPORT int plan_sugerido(int max_creditos, int max_dificultad, int *asignacion) {
    if (g_cursos == NULL) {
        return -1;
    }
    return semestre_sugerido(g_cursos, g_n, max_creditos, max_dificultad, asignacion);
}

/* ------------------------------------------------------------------ */
/* Materias restantes (para voraz y backtracking)                      */
/* ------------------------------------------------------------------ */

/*
 * Arma un arreglo solo con las materias NO aprobadas. Los prerrequisitos ya
 * aprobados se quitan y los demas se reindexan. mapa[k] guarda el indice
 * original de la materia k del arreglo reducido.
 */
static Curso *crear_restantes(const char *aprobadas, int *m, int **mapa_out) {
    int restantes = 0;
    for (int i = 0; i < g_n; i++) {
        if (aprobadas == NULL || !aprobadas[i]) {
            restantes++;
        }
    }
    *m = restantes;
    *mapa_out = NULL;
    if (restantes == 0) {
        return NULL;
    }

    int *nuevo = (int *)malloc((size_t)g_n * sizeof(int));
    int *mapa = (int *)malloc((size_t)restantes * sizeof(int));
    Curso *sub = (Curso *)malloc((size_t)restantes * sizeof(Curso));
    if (nuevo == NULL || mapa == NULL || sub == NULL) {
        free(nuevo);
        free(mapa);
        free(sub);
        *m = -1;
        return NULL;
    }

    int k = 0;
    for (int i = 0; i < g_n; i++) {
        if (aprobadas == NULL || !aprobadas[i]) {
            nuevo[i] = k;
            mapa[k] = i;
            k++;
        } else {
            nuevo[i] = -1;
        }
    }

    for (k = 0; k < restantes; k++) {
        const Curso *c = &g_cursos[mapa[k]];
        int pre[64];
        int np = 0;
        for (int j = 0; j < c->num_prerequisitos && np < 64; j++) {
            int p = c->prerequisitos[j];
            if (p >= 0 && p < g_n && p != mapa[k] && nuevo[p] >= 0) {
                pre[np++] = nuevo[p];
            }
        }
        crear_curso(&sub[k], c->nombre, c->creditos, c->semestre, c->dificultad,
                    np > 0 ? pre : NULL, np);
    }

    free(nuevo);
    *mapa_out = mapa;
    return sub;
}


/*
criterio: 0 = desbloquea (recomendado), 1 = solo creditos.
asignacion[i] = semestre (1, 2, ...) de la materia i, o 0 si ya estaba
aprobada. Devuelve semestres usados, o -1 si hay ciclo / falla memoria.
 */
EXPORT int plan_voraz(int criterio, int max_creditos, int max_materias,
                      const char *aprobadas, int *asignacion) {
    if (g_cursos == NULL) {
        return -1;
    }
    for (int i = 0; i < g_n; i++) {
        asignacion[i] = 0;
    }

    int m = 0;
    int *mapa = NULL;
    Curso *sub = crear_restantes(aprobadas, &m, &mapa);
    if (m < 0) {
        return -1;
    }
    if (m == 0) {
        return 0;
    }

    int *asig_sub = (int *)calloc((size_t)m, sizeof(int));
    if (asig_sub == NULL) {
        liberar_arreglo_cursos(sub, m);
        free(mapa);
        return -1;
    }

    CriterioVoraz cv = (criterio == 1) ? VORAZ_CREDITOS : VORAZ_DESBLOQUEA;
    int semestres = seleccion_voraz(sub, m, max_creditos, max_materias, cv, asig_sub);

    if (semestres >= 0) {
        for (int k = 0; k < m; k++) {
            asignacion[mapa[k]] = asig_sub[k];
        }
    }

    free(asig_sub);
    free(mapa);
    liberar_arreglo_cursos(sub, m);
    return semestres;
}



/*
Devuelve el codigo de plan_backtracking: 0 = hay plan, 1 = no se hallo plan
dentro del limite, -1 = ciclo o memoria, -2 = mas de 64 materias.
asignacion[i] = semestre (1..) o 0 si ya estaba aprobada
pasos[0]      = nodos de busqueda visitados
info[0] = semestres del mejor plan
info[1] = 1 si se corto por limite de pasos
info[2] = 1 si el minimo quedo demostrado
info[3] = semestres del primer plan hallado
*/
EXPORT int plan_bt(const char *aprobadas, int max_creditos, int max_materias,
                   long long limite_pasos, int *asignacion, long long *pasos,
                   int *info) {
    ResultadoPlan res;
    memset(&res, 0, sizeof(res));
    for (int i = 0; i < g_n; i++) {
        asignacion[i] = 0;
    }
    pasos[0] = 0;
    for (int k = 0; k < 4; k++) {
        info[k] = 0;
    }
    if (g_cursos == NULL) {
        return -1;
    }

    int r = plan_backtracking(g_cursos, g_n, aprobadas, max_creditos,
                              max_materias, limite_pasos, &res);
    if (r == 0 || r == 1) {
        pasos[0] = res.pasos;
        info[0] = res.semestres;
        info[1] = res.truncado;
        info[2] = res.optimo_demostrado;
        info[3] = res.primer_plan_semestres;
        if (res.encontrado && res.asignacion != NULL) {
            for (int i = 0; i < g_n; i++) {
                asignacion[i] = res.asignacion[i];
            }
        }
    }
    liberar_resultado_plan(&res);
    return r;
}
