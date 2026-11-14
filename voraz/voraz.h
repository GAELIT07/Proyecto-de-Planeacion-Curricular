#ifndef VORAZ_H
#define VORAZ_H

#include "curso.h"

// En cada semestre se toman las materias elegibles (todas sus prerrequisitos
// ya cursados en semestres anteriores) y se escogen de forma voraz segun un
// criterio de prioridad, hasta llenar el tope de creditos del semestre.
// Los prerrequisitos son INDICES (base 0) sobre el arreglo original, igual
// que en el resto del proyecto. Los indices fuera de rango o que apuntan al
// mismo curso se ignoran.

typedef enum {
    
    VORAZ_DESBLOQUEA,
   
    VORAZ_CREDITOS
} CriterioVoraz;

typedef struct {
    int *desbloquea;     // desbloquea[i]: materias que dependen de i, directa o indirectamente
    int *ruta_critica;   // ruta_critica[i]: longitud de la cadena mas larga que cuelga de i 
} MetricasVoraz;

// Calcula las metricas. Devuelve 0 si todo bien, -1 si hay ciclo de
// prerrequisitos o falla la memoria.
int calcular_metricas_voraz(const Curso *cursos, int n, MetricasVoraz *m);
void liberar_metricas_voraz(MetricasVoraz *m);

// Llena asignacion[i] (arreglo de n enteros) con el semestre (1, 2, ...) en
// que se cursa el curso i, y devuelve el numero de semestres usados.
// Una materia que por si sola excede el tope se cursa sola en un semestre
// (para no bloquear el plan). Devuelve -1 si hay ciclo de prerrequisitos.
int seleccion_voraz(const Curso *cursos, int n, int max_creditos,
                    int max_materias, CriterioVoraz criterio, int *asignacion);

// Imprime el plan por semestre con la justificacion de cada seleccion
// (cuantas materias desbloquea, ruta critica y creditos).
void imprimir_plan_voraz(const Curso *cursos, int n, const int *asignacion,
                         int num_semestres, const MetricasVoraz *m,
                         const char *titulo);

#endif // VORAZ_H
