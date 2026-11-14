#include "backtracking.h"
#include "voraz.h"
#include "quicksort.h"
#include <limits.h>

#define BITS_MEMO 19
#define TAM_MEMO (1u << BITS_MEMO)

typedef unsigned long long u64;

typedef struct {
    const Curso *c;
    int n;
    u64 *prer;            // prer[i]: mascara de prerrequisitos validos de i
    u64 full;             // mascara con las n materias
    MetricasVoraz m;      // desbloquea / ruta critica (se reutiliza de U3)
    int max_cred, max_mat;
    int creditos_caben;   // 1 si ninguna materia excede max_cred por si sola
    long long limite, pasos;
    int truncado;
    int *cur, *best;
    int best_sem, primer_sem;
    u64 *memo_k;
    int *memo_v;
} Ctx;

static int caben(const Ctx *x, int cred, int cnt, int c) {
    if (cnt == 0) {
        return 1;   // la primera siempre entra (evita bloquear el plan)
    }
    if (x->max_mat > 0 && cnt >= x->max_mat) {
        return 0;
    }
    if (x->max_cred > 0 && cred + x->c[c].creditos > x->max_cred) {
        return 0;
    }
    return 1;
}

// > 0 si a debe probarse antes que b (solo ordena las ramas)
static int mejor(const Ctx *x, int a, int b) {
    if (x->m.desbloquea[a] != x->m.desbloquea[b]) return x->m.desbloquea[a] - x->m.desbloquea[b];
    if (x->m.ruta_critica[a] != x->m.ruta_critica[b]) return x->m.ruta_critica[a] - x->m.ruta_critica[b];
    if (x->c[a].creditos != x->c[b].creditos) return x->c[a].creditos - x->c[b].creditos;
    return b - a;
}

// Adaptador para quicksort_indices: la rama "mejor" se prueba primero
static int comparar_ramas(int a, int b, const void *ctx) {
    return -mejor((const Ctx *)ctx, a, b);
}

// Cota inferior de semestres que aun faltan (nunca sobreestima)
static int cota_inferior(const Ctx *x, u64 mask) {
    int cnt = 0, cred = 0, cadena = 0;
    for (int i = 0; i < x->n; i++) {
        if (!((mask >> i) & 1ULL)) {
            cnt++;
            cred += x->c[i].creditos;
            if (x->m.ruta_critica[i] > cadena) cadena = x->m.ruta_critica[i];
        }
    }
    int lb = cadena;
    if (x->max_mat > 0) {
        int v = (cnt + x->max_mat - 1) / x->max_mat;
        if (v > lb) lb = v;
    }
    if (x->max_cred > 0 && x->creditos_caben) {
        int v = (cred + x->max_cred - 1) / x->max_cred;
        if (v > lb) lb = v;
    }
    return lb;
}

// 1 si este conjunto de aprobadas ya se alcanzo en un semestre <= sem
static int ya_visto(Ctx *x, u64 mask, int sem) {
    unsigned idx = (unsigned)((mask * 0x9E3779B97F4A7C15ULL) >> (64 - BITS_MEMO));
    for (int probe = 0; probe < 64; probe++) {
        unsigned i = (idx + (unsigned)probe) & (TAM_MEMO - 1);
        if (x->memo_v[i] == 0) {
            x->memo_k[i] = mask;
            x->memo_v[i] = sem;
            return 0;
        }
        if (x->memo_k[i] == mask) {
            if (x->memo_v[i] <= sem) return 1;
            x->memo_v[i] = sem;
            return 0;
        }
    }
    return 0;   // tabla saturada en esta zona: simplemente no se poda
}

static void semestre_rec(Ctx *x, u64 mask, int sem);

static void elegir_rec(Ctx *x, const int *E, int ne, int k, u64 mask, int sem,
                       u64 sel, int cred, int cnt) {
    if (x->truncado) return;
    if (++x->pasos > x->limite) { x->truncado = 1; return; }

    if (k == ne) {
        if (cnt == 0) return;
        // Maximal: ninguna elegible que quedo fuera debe caber todavia
        for (int q = 0; q < ne; q++) {
            if (!((sel >> E[q]) & 1ULL) && caben(x, cred, cnt, E[q])) return;
        }
        semestre_rec(x, mask | sel, sem + 1);
        return;
    }

    int c = E[k];
    if (caben(x, cred, cnt, c)) {
        x->cur[c] = sem;
        elegir_rec(x, E, ne, k + 1, mask, sem, sel | (1ULL << c), cred + x->c[c].creditos, cnt + 1);
        x->cur[c] = 0;
    }
    elegir_rec(x, E, ne, k + 1, mask, sem, sel, cred, cnt);
}

static void semestre_rec(Ctx *x, u64 mask, int sem) {
    if (x->truncado) return;
    if (++x->pasos > x->limite) { x->truncado = 1; return; }

    if (mask == x->full) {
        int usados = sem - 1;
        if (x->primer_sem == 0) x->primer_sem = usados;
        if (usados < x->best_sem) {
            x->best_sem = usados;
            memcpy(x->best, x->cur, (size_t)x->n * sizeof(int));
        }
        return;
    }
    if (sem - 1 + cota_inferior(x, mask) >= x->best_sem) return;   // no puede mejorar
    if (ya_visto(x, mask, sem)) return;

    int *E = (int *)malloc((size_t)x->n * sizeof(int));
    if (E == NULL) { x->truncado = 1; return; }
    int ne = 0;
    for (int i = 0; i < x->n; i++) {
        if (!((mask >> i) & 1ULL) && (x->prer[i] & ~mask) == 0) {
            E[ne++] = i;
        }
    }
    // Ordenar ramas con quicksort (modulo U2): solo afecta la velocidad
    quicksort_indices(E, ne, comparar_ramas, x);
    if (ne > 0) {
        elegir_rec(x, E, ne, 0, mask, sem, 0ULL, 0, 0);
    }
    free(E);
}

int plan_backtracking(const Curso *cursos, int n, const char *aprobadas,
                      int max_creditos, int max_materias,
                      long long limite_pasos, ResultadoPlan *res) {
    memset(res, 0, sizeof(*res));
    if (n > 64) return -2;
    res->asignacion = (int *)calloc((size_t)(n > 0 ? n : 1), sizeof(int));
    if (res->asignacion == NULL) return -1;

    Ctx x;
    memset(&x, 0, sizeof(x));
    x.c = cursos;
    x.n = n;
    x.max_cred = max_creditos;
    x.max_mat = max_materias;
    x.limite = limite_pasos > 0 ? limite_pasos : LLONG_MAX;
    x.best_sem = INT_MAX;
    x.full = (n == 64) ? ~0ULL : ((1ULL << n) - 1ULL);

    if (calcular_metricas_voraz(cursos, n, &x.m) != 0) return -1;   // ciclo o memoria

    x.prer = (u64 *)calloc((size_t)n, sizeof(u64));
    x.cur = (int *)calloc((size_t)n, sizeof(int));
    x.best = (int *)calloc((size_t)n, sizeof(int));
    x.memo_k = (u64 *)calloc(TAM_MEMO, sizeof(u64));
    x.memo_v = (int *)calloc(TAM_MEMO, sizeof(int));
    if (!x.prer || !x.cur || !x.best || !x.memo_k || !x.memo_v) {
        free(x.prer); free(x.cur); free(x.best); free(x.memo_k); free(x.memo_v);
        liberar_metricas_voraz(&x.m);
        return -1;
    }

    x.creditos_caben = 1;
    u64 mask0 = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < cursos[i].num_prerequisitos; j++) {
            int p = cursos[i].prerequisitos[j];
            if (p >= 0 && p < n && p != i) x.prer[i] |= (1ULL << p);
        }
        if (max_creditos > 0 && cursos[i].creditos > max_creditos) x.creditos_caben = 0;
        if (aprobadas != NULL && aprobadas[i]) mask0 |= (1ULL << i);
    }

    semestre_rec(&x, mask0, 1);

    res->pasos = x.pasos;
    res->truncado = x.truncado;
    res->primer_plan_semestres = x.primer_sem;
    if (x.best_sem != INT_MAX) {
        res->encontrado = 1;
        res->semestres = x.best_sem;
        memcpy(res->asignacion, x.best, (size_t)n * sizeof(int));
        res->optimo_demostrado = !x.truncado;
    }

    free(x.prer); free(x.cur); free(x.best); free(x.memo_k); free(x.memo_v);
    liberar_metricas_voraz(&x.m);
    return res->encontrado ? 0 : 1;
}

void liberar_resultado_plan(ResultadoPlan *res) {
    free(res->asignacion);
    res->asignacion = NULL;
}

void imprimir_plan_backtracking(const Curso *cursos, int n, const ResultadoPlan *res) {
    printf("\n============================================================\n");
    printf("Plan por backtracking (busqueda exhaustiva con poda)\n");
    printf("============================================================\n");
    if (!res->encontrado) {
        printf("No se encontro un plan dentro del limite de pasos (%lld).\n", res->pasos);
        return;
    }
    for (int s = 1; s <= res->semestres; s++) {
        int cant = 0, cred = 0;
        for (int i = 0; i < n; i++) {
            if (res->asignacion[i] == s) { cant++; cred += cursos[i].creditos; }
        }
        printf("\nSemestre %d | %d materias | %d creditos\n", s, cant, cred);
        for (int i = 0; i < n; i++) {
            if (res->asignacion[i] == s) {
                printf("  - %-50s cr:%d\n", cursos[i].nombre, cursos[i].creditos);
            }
        }
    }
    printf("\nSemestres del plan: %d | primer plan hallado: %d | pasos de busqueda: %lld\n",
           res->semestres, res->primer_plan_semestres, res->pasos);
    if (res->optimo_demostrado) {
        printf("[EXACTO] Busqueda completa: %d es el MINIMO de semestres para esta instancia\n"
               "         y este modelo (topes de semestre y prerrequisitos dados).\n", res->semestres);
    } else {
        printf("[FACTIBLE, NO OPTIMO DEMOSTRADO] La busqueda se corto por limite de pasos.\n"
               "         El plan es valido, pero NO se afirma que %d sea el minimo.\n", res->semestres);
    }
}
