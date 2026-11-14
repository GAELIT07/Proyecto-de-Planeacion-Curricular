# -*- coding: utf-8 -*-
"""
Proyecto - Planeación curricular (interfaz gráfica).

La interfaz está hecha con Tkinter y usa el código en C (cursos.dll /
libcursos.so) para todo el cálculo: quicksort, materias elegibles, validación
de la malla, plan sugerido, selección voraz y backtracking.

Novedades de esta versión:
  * Cada materia puede estar APROBADA, REPROBADA o PENDIENTE (se guarda en
    historial.json junto a este archivo).
  * Al ordenar, las materias REPROBADAS van primero (lo hace el quicksort en C
    con ordenar_cursos_prioridad) y el resultado se dibuja como MAPA
    CURRICULAR: una columna por semestre.
  * Al hacer clic en una materia se resalta su ruta de prerrequisitos.
  * Accesibilidad: botones de tamaño de texto (A- / A+), colores con buen
    contraste, el estado nunca se indica solo con color (siempre hay un símbolo
    y un texto), todo se puede usar con teclado (Tab, Enter y Espacio) y la
    ventana se adapta al tamaño de la pantalla.
"""

import ctypes
import json
import os
import sys
import threading
import tkinter as tk
import unicodedata
from tkinter import font as tkfont
from tkinter import messagebox, ttk

CARPETA = os.path.dirname(os.path.abspath(__file__))

# ======================================================================
# COLORES Y FUENTES
# ======================================================================

AZUL_OSCURO = "#172554"
AZUL = "#1D4ED8"
FONDO = "#F1F5F9"
BLANCO = "#FFFFFF"
TEXTO = "#0F172A"
TEXTO_SUAVE = "#475569"
BORDE = "#CBD5E1"
VERDE = "#166534"
VERDE_FONDO = "#DCFCE7"
ROJO = "#991B1B"
ROJO_FONDO = "#FEE2E2"
AMBAR = "#92400E"
AMBAR_FONDO = "#FEF3C7"

# Cada estado se distingue por color, símbolo y texto (no solo por color).
PALETA = {
    "aprobada": dict(fondo="#DCFCE7", borde="#15803D", texto="#14532D",
                     etiqueta="✓ Aprobada"),
    "reprobada": dict(fondo="#FEE2E2", borde="#B91C1C", texto="#7F1D1D",
                      etiqueta="✗ Reprobada"),
    "disponible": dict(fondo="#DBEAFE", borde="#1D4ED8", texto="#1E3A8A",
                       etiqueta="● Disponible"),
    "bloqueada": dict(fondo="#FFFFFF", borde="#94A3B8", texto="#334155",
                      etiqueta="○ Bloqueada"),
    "normal": dict(fondo="#FFFFFF", borde="#94A3B8", texto="#0F172A",
                   etiqueta="= Sin cambio"),
    "movida": dict(fondo="#FEF3C7", borde="#B45309", texto="#78350F",
                   etiqueta="↪ Movida de semestre"),
    "atenuada": dict(fondo="#F8FAFC", borde="#E2E8F0", texto="#94A3B8",
                     etiqueta=""),
}

COLOR_SELECCION = "#0F172A"
COLOR_REQUISITO = "#7C3AED"    # materias que hay que aprobar antes (↑)
COLOR_DESBLOQUEA = "#C2410C"   # materias que dependen de la elegida (↓)

FAMILIA = "Segoe UI"
ESCALAS = [0.9, 1.0, 1.15, 1.3, 1.5]
ESCALA = 1.0


def fuente(tamano, peso="normal"):
    """Fuente con el tamaño de texto elegido por la persona."""
    return (FAMILIA, max(7, int(round(tamano * ESCALA))), peso)


# ======================================================================
# VENTANA
# ======================================================================

ventana = tk.Tk()
ventana.title("Proyecto - Planeación curricular")

_ancho_pantalla = ventana.winfo_screenwidth()
_alto_pantalla = ventana.winfo_screenheight()
_ancho = min(1600, int(_ancho_pantalla * 0.94))
_alto = min(950, int(_alto_pantalla * 0.92))
ventana.geometry(
    f"{_ancho}x{_alto}+{(_ancho_pantalla - _ancho) // 2}"
    f"+{max((_alto_pantalla - _alto) // 2 - 20, 0)}"
)
ventana.minsize(960, 620)
ventana.configure(bg=FONDO)

if sys.platform.startswith("win"):
    try:
        ventana.state("zoomed")          # maximizada en Windows
    except tk.TclError:
        pass

# ======================================================================
# CARGA DE DATOS DESDE C
# ======================================================================

BIBLIOTECA = None          # biblioteca de C ya cargada
USA_PRIORIDAD_C = False    # True si la DLL trae ordenar_cursos_prioridad


def decodificar(texto_bytes):
    """Convierte el texto que entrega C (UTF-8, o ANSI de Windows)."""
    try:
        return texto_bytes.decode("utf-8")
    except UnicodeDecodeError:
        return texto_bytes.decode("cp1252", errors="replace")


def configurar_funciones_extra(biblioteca):
    """Define los tipos de las funciones del puente que no son de carga."""
    global USA_PRIORIDAD_C
    arreglo_int = ctypes.POINTER(ctypes.c_int)

    biblioteca.ordenar_cursos.argtypes = [
        ctypes.c_int, ctypes.c_int, arreglo_int
    ]
    biblioteca.ordenar_cursos.restype = ctypes.c_int

    # Ordenamiento con prioridad a reprobadas (versión nueva de la DLL).
    # Si la DLL es anterior y no la trae, se usa un respaldo en Python.
    try:
        biblioteca.ordenar_cursos_prioridad.argtypes = [
            ctypes.c_int, ctypes.c_int, ctypes.c_char_p, arreglo_int
        ]
        biblioteca.ordenar_cursos_prioridad.restype = ctypes.c_int
        USA_PRIORIDAD_C = True
    except AttributeError:
        USA_PRIORIDAD_C = False

    biblioteca.calcular_elegibles.argtypes = [ctypes.c_char_p, arreglo_int]
    biblioteca.calcular_elegibles.restype = ctypes.c_int

    biblioteca.validar_indices_malla.argtypes = []
    biblioteca.validar_indices_malla.restype = ctypes.c_int

    biblioteca.buscar_ciclos.argtypes = []
    biblioteca.buscar_ciclos.restype = ctypes.c_int

    biblioteca.obtener_ciclo.argtypes = [
        ctypes.c_int, arreglo_int, ctypes.c_int
    ]
    biblioteca.obtener_ciclo.restype = ctypes.c_int

    biblioteca.plan_sugerido.argtypes = [
        ctypes.c_int, ctypes.c_int, arreglo_int
    ]
    biblioteca.plan_sugerido.restype = ctypes.c_int

    biblioteca.plan_voraz.argtypes = [
        ctypes.c_int, ctypes.c_int, ctypes.c_int,
        ctypes.c_char_p, arreglo_int
    ]
    biblioteca.plan_voraz.restype = ctypes.c_int

    biblioteca.plan_bt.argtypes = [
        ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_longlong,
        arreglo_int, ctypes.POINTER(ctypes.c_longlong), arreglo_int
    ]
    biblioteca.plan_bt.restype = ctypes.c_int


def cargar_materias_c():
    """Carga la biblioteca de C y lee cursos.txt. Devuelve la lista de
    materias como tuplas (nombre, créditos, semestre, dificultad,
    prerrequisitos)."""
    global BIBLIOTECA
    nombre_dll = "cursos.dll" if sys.platform.startswith("win") else "libcursos.so"
    ruta_dll = os.path.join(CARPETA, nombre_dll)
    ruta_archivo = os.path.join(CARPETA, "cursos.txt")

    # Si no hay cursos.txt junto a main.py, se usa el del proyecto
    if not os.path.exists(ruta_archivo):
        alterna = os.path.join(CARPETA, "..", "quicksort", "cursos.txt")
        if os.path.exists(alterna):
            ruta_archivo = os.path.normpath(alterna)

    biblioteca = ctypes.CDLL(ruta_dll)

    biblioteca.cargar_cursos.argtypes = [ctypes.c_char_p]
    biblioteca.cargar_cursos.restype = ctypes.c_int
    biblioteca.obtener_total_cursos.restype = ctypes.c_int
    biblioteca.obtener_nombre.argtypes = [ctypes.c_int]
    biblioteca.obtener_nombre.restype = ctypes.c_char_p
    biblioteca.obtener_creditos.argtypes = [ctypes.c_int]
    biblioteca.obtener_creditos.restype = ctypes.c_int
    biblioteca.obtener_semestre.argtypes = [ctypes.c_int]
    biblioteca.obtener_semestre.restype = ctypes.c_int
    biblioteca.obtener_dificultad.argtypes = [ctypes.c_int]
    biblioteca.obtener_dificultad.restype = ctypes.c_int
    biblioteca.obtener_num_prerequisitos.argtypes = [ctypes.c_int]
    biblioteca.obtener_num_prerequisitos.restype = ctypes.c_int
    biblioteca.obtener_prerequisito.argtypes = [ctypes.c_int, ctypes.c_int]
    biblioteca.obtener_prerequisito.restype = ctypes.c_int
    biblioteca.liberar_cursos.argtypes = []
    biblioteca.liberar_cursos.restype = None

    configurar_funciones_extra(biblioteca)

    total = biblioteca.cargar_cursos(os.fsencode(ruta_archivo))
    if total <= 0:
        raise RuntimeError("C no pudo cargar las materias de cursos.txt")

    cargadas = []
    try:
        for i in range(total):
            nombre = decodificar(biblioteca.obtener_nombre(i))
            cantidad = biblioteca.obtener_num_prerequisitos(i)
            prerrequisitos = [
                biblioteca.obtener_prerequisito(i, j) for j in range(cantidad)
            ]
            cargadas.append((
                nombre,
                biblioteca.obtener_creditos(i),
                biblioteca.obtener_semestre(i),
                biblioteca.obtener_dificultad(i),
                prerrequisitos
            ))
    except Exception:
        biblioteca.liberar_cursos()
        raise

    BIBLIOTECA = biblioteca
    return cargadas


error_carga = None
try:
    materias = cargar_materias_c()
    print("Materias cargadas:", len(materias))
except (OSError, RuntimeError, UnicodeDecodeError) as _error:
    materias = []
    error_carga = str(_error)
    print("Error al cargar las materias:", _error)

# ======================================================================
# ESTADO DE CADA MATERIA (aprobada / reprobada / pendiente)
# ======================================================================

estado = ["pendiente"] * len(materias)       # un estado por materia (por índice)
ARCHIVO_HISTORIAL = os.path.join(CARPETA, "historial.json")

DEPENDIENTES = [[] for _ in materias]        # DEPENDIENTES[p]: materias que piden a p
for _i, _m in enumerate(materias):
    for _p in _m[4]:
        if 0 <= _p < len(materias) and _p != _i:
            DEPENDIENTES[_p].append(_i)


def guardar_historial():
    datos = {
        "aprobadas": [materias[i][0] for i, e in enumerate(estado)
                      if e == "aprobada"],
        "reprobadas": [materias[i][0] for i, e in enumerate(estado)
                       if e == "reprobada"],
    }
    try:
        with open(ARCHIVO_HISTORIAL, "w", encoding="utf-8") as archivo:
            json.dump(datos, archivo, ensure_ascii=False, indent=2)
    except OSError:
        pass        # si no se puede guardar, la app sigue funcionando


def cargar_historial():
    try:
        with open(ARCHIVO_HISTORIAL, "r", encoding="utf-8") as archivo:
            datos = json.load(archivo)
        aprobadas = set(datos.get("aprobadas") or [])
        reprobadas = set(datos.get("reprobadas") or [])
    except (OSError, ValueError, AttributeError, TypeError):
        return
    for i, materia in enumerate(materias):
        if materia[0] in aprobadas:
            estado[i] = "aprobada"
        elif materia[0] in reprobadas:
            estado[i] = "reprobada"


cargar_historial()


def cambiar_estado(indice, nuevo):
    estado[indice] = nuevo
    guardar_historial()


def cuenta_estado(nombre_estado):
    return sum(1 for e in estado if e == nombre_estado)


# ======================================================================
# FUNCIONES QUE USAN EL CÓDIGO EN C
# ======================================================================

MAX_CREDITOS_SEMESTRE = 50
MAX_DIFICULTAD_SEMESTRE = 38
LIMITE_PASOS_BACKTRACKING = 3000000

CAMPOS_ORDENAMIENTO = {
    "Créditos": 0,
    "Dificultad": 1,
    "Semestre": 2,
    "Nombre": 3,
}


def hay_biblioteca():
    return BIBLIOTECA is not None and len(materias) > 0


def nuevo_arreglo_c(tamano):
    return (ctypes.c_int * max(tamano, 1))()


def normalizar(texto):
    """Minúsculas y sin acentos, para buscar 'calculo' y encontrar 'Cálculo'."""
    sin_acentos = "".join(
        c for c in unicodedata.normalize("NFD", texto)
        if unicodedata.category(c) != "Mn"
    )
    return sin_acentos.lower()


def nombres_prerrequisitos(indices):
    nombres = [materias[i][0] for i in indices if 0 <= i < len(materias)]
    return ", ".join(nombres) if nombres else "Ninguno"


def construir_aprobadas():
    """Un byte por materia (1 = aprobada), en el orden de la malla."""
    return bytes(1 if e == "aprobada" else 0 for e in estado)


def construir_reprobadas():
    """Un byte por materia (1 = reprobada), en el orden de la malla."""
    return bytes(1 if e == "reprobada" else 0 for e in estado)


def creditos_por_semestre(asignacion):
    totales = {}
    for indice, semestre in enumerate(asignacion):
        if semestre > 0:
            totales[semestre] = totales.get(semestre, 0) + materias[indice][1]
    return totales


def ordenar_materias_c(campo, descendente=False, prioridad_reprobadas=False):
    """Quicksort en C. Devuelve (indices ya ordenados, motor usado).

    Con prioridad_reprobadas=True las materias reprobadas van primero y
    dentro de cada grupo se respeta el criterio elegido.
    """
    if not hay_biblioteca():
        return [], "C"
    salida = nuevo_arreglo_c(len(materias))
    desc = 1 if descendente else 0

    if prioridad_reprobadas and USA_PRIORIDAD_C:
        total = BIBLIOTECA.ordenar_cursos_prioridad(
            campo, desc, construir_reprobadas(), salida
        )
        motor = "C"
    else:
        total = BIBLIOTECA.ordenar_cursos(campo, desc, salida)
        motor = "C"

    if total < 0:
        raise RuntimeError("C no pudo ordenar las materias")
    orden = [salida[k] for k in range(total)]

    if prioridad_reprobadas and not USA_PRIORIDAD_C:
        # Respaldo para una cursos.dll anterior: da exactamente el mismo
        # resultado (el orden base es total, no hay empates).
        orden = ([i for i in orden if estado[i] == "reprobada"]
                 + [i for i in orden if estado[i] != "reprobada"])
        motor = "Python"
    return orden, motor


def elegibles_c():
    """Índices de las materias que se pueden cursar con el historial."""
    if not hay_biblioteca():
        return []
    salida = nuevo_arreglo_c(len(materias))
    total = BIBLIOTECA.calcular_elegibles(construir_aprobadas(), salida)
    return [salida[k] for k in range(total)]


def validar_malla_c():
    """Devuelve (indices_invalidos, ciclos)."""
    if not hay_biblioteca():
        return 0, []
    invalidos = BIBLIOTECA.validar_indices_malla()
    total = BIBLIOTECA.buscar_ciclos()
    ciclos = []
    for k in range(max(min(total, 16), 0)):
        nodos = nuevo_arreglo_c(128)
        largo = BIBLIOTECA.obtener_ciclo(k, nodos, 128)
        ciclos.append([nodos[j] for j in range(largo)])
    return invalidos, ciclos


def plan_sugerido_c(max_creditos, max_dificultad):
    if not hay_biblioteca():
        return -1, []
    asignacion = nuevo_arreglo_c(len(materias))
    semestres = BIBLIOTECA.plan_sugerido(
        max_creditos, max_dificultad, asignacion
    )
    return semestres, [asignacion[i] for i in range(len(materias))]


def plan_voraz_c(criterio, max_creditos):
    if not hay_biblioteca():
        return -1, []
    asignacion = nuevo_arreglo_c(len(materias))
    semestres = BIBLIOTECA.plan_voraz(
        criterio, max_creditos, 0, construir_aprobadas(), asignacion
    )
    return semestres, [asignacion[i] for i in range(len(materias))]


def plan_backtracking_c(max_creditos, aprobadas):
    """Backtracking en C. Se puede llamar desde otro hilo."""
    resultado = {
        "codigo": -1, "semestres": 0, "truncado": 0,
        "optimo": 0, "pasos": 0, "asignacion": []
    }
    if not hay_biblioteca():
        return resultado
    asignacion = nuevo_arreglo_c(len(materias))
    pasos = (ctypes.c_longlong * 1)()
    info = nuevo_arreglo_c(4)
    resultado["codigo"] = BIBLIOTECA.plan_bt(
        aprobadas, max_creditos, 0, LIMITE_PASOS_BACKTRACKING,
        asignacion, pasos, info
    )
    resultado["semestres"] = info[0]
    resultado["truncado"] = info[1]
    resultado["optimo"] = info[2]
    resultado["pasos"] = pasos[0]
    resultado["asignacion"] = [asignacion[i] for i in range(len(materias))]
    return resultado


# ---- Utilidades sobre la malla (solo Python) ---------------------------

def _recorrer(inicio, vecinos):
    visitados = set()
    pila = list(vecinos(inicio))
    while pila:
        j = pila.pop()
        if j in visitados or j == inicio:
            continue
        visitados.add(j)
        pila.extend(vecinos(j))
    return visitados


def ancestros(i):
    """Todas las materias que hay que aprobar antes de i (directas o no)."""
    return _recorrer(
        i, lambda k: [p for p in materias[k][4] if 0 <= p < len(materias)]
    )


def descendientes(i):
    """Todas las materias que dependen de i (directa o indirectamente)."""
    return _recorrer(i, lambda k: DEPENDIENTES[k])


def prerrequisitos_sin_aprobar(i):
    return [
        materias[p][0] for p in materias[i][4]
        if 0 <= p < len(materias) and p != i and estado[p] != "aprobada"
    ]


def incoherencias():
    """Avisos de historial que no tiene sentido (p. ej. una materia
    aprobada o reprobada cuyo prerrequisito no está aprobado)."""
    avisos = []
    for i, materia in enumerate(materias):
        if estado[i] == "pendiente":
            continue
        faltan = prerrequisitos_sin_aprobar(i)
        if faltan:
            palabra = "aprobada" if estado[i] == "aprobada" else "reprobada"
            avisos.append(
                f"{materia[0]} figura como {palabra}, pero falta aprobar: "
                + ", ".join(faltan)
            )
    return avisos


def estado_visual(i, disponibles):
    if estado[i] in ("aprobada", "reprobada"):
        return estado[i]
    return "disponible" if i in disponibles else "bloqueada"


def resumen_columna(indices):
    creditos = sum(materias[i][1] for i in indices)
    dificultad = sum(materias[i][3] for i in indices)
    return f"{len(indices)} materias · {creditos} cr · dif. {dificultad}"


# ======================================================================
# ESTILOS Y ESTRUCTURA PRINCIPAL
# ======================================================================

estilo = ttk.Style()
estilo.theme_use("clam")


def aplicar_estilos():
    for nombre in ("TkDefaultFont", "TkTextFont", "TkMenuFont",
                   "TkHeadingFont"):
        try:
            tkfont.nametofont(nombre).configure(
                family=FAMILIA, size=max(7, int(round(10 * ESCALA)))
            )
        except tk.TclError:
            pass

    estilo.configure(
        "Treeview",
        background=BLANCO,
        foreground=TEXTO,
        rowheight=int(34 * ESCALA),
        fieldbackground=BLANCO,
        font=fuente(10)
    )
    estilo.configure(
        "Treeview.Heading",
        font=fuente(10, "bold"),
        background="#E2E8F0",
        foreground=TEXTO,
        padding=6
    )
    # Con este map los colores de las etiquetas (tags) se ven bien en todas
    # las versiones de Tk.
    estilo.map(
        "Treeview",
        background=[("selected", AZUL)],
        foreground=[("selected", BLANCO)]
    )
    estilo.configure("TCombobox", padding=5)
    estilo.configure(
        "Avance.Horizontal.TProgressbar",
        troughcolor="#E2E8F0",
        background=VERDE,
        thickness=int(18 * ESCALA)
    )


aplicar_estilos()

menu_lateral = tk.Frame(ventana, bg=AZUL_OSCURO, width=240)
menu_lateral.pack(side="left", fill="y")
menu_lateral.pack_propagate(False)

contenido = tk.Frame(ventana, bg=FONDO)
contenido.pack(side="right", fill="both", expand=True)

pantalla_actual = None          # función de la pantalla que se está mostrando
botones_menu = {}               # función de pantalla -> botón del menú
area_desplazable = [None]       # canvas que recibe la rueda del ratón


# ----------------------------------------------------------------------
# Rueda del ratón (una sola vez para toda la aplicación)
# ----------------------------------------------------------------------

def _rueda_global(evento):
    canvas = area_desplazable[0]
    if canvas is None:
        return
    try:
        if evento.widget.winfo_class() in ("Treeview", "Listbox"):
            return          # estos widgets ya se desplazan solos
    except (AttributeError, tk.TclError):
        pass

    numero = getattr(evento, "num", 0)
    if numero == 4:
        paso = -1
    elif numero == 5:
        paso = 1
    else:
        paso = -1 if getattr(evento, "delta", 0) > 0 else 1

    try:
        if getattr(evento, "state", 0) & 0x0001:       # Shift = horizontal
            canvas.xview_scroll(paso, "units")
        else:
            canvas.yview_scroll(paso, "units")
    except tk.TclError:
        area_desplazable[0] = None


ventana.bind_all("<MouseWheel>", _rueda_global)
ventana.bind_all("<Button-4>", _rueda_global)
ventana.bind_all("<Button-5>", _rueda_global)


# ======================================================================
# COMPONENTES REUTILIZABLES
# ======================================================================

ESTILOS_BOTON = {
    # tipo: (fondo, texto, fondo al presionar)
    "primario": (AZUL, BLANCO, "#1E40AF"),
    "secundario": ("#E2E8F0", TEXTO, "#CBD5E1"),
    "claro": ("#EFF6FF", AZUL, "#DBEAFE"),
    "aprobada": (VERDE_FONDO, VERDE, "#BBF7D0"),
    "reprobada": (ROJO_FONDO, ROJO, "#FECACA"),
}


def boton(padre, texto, comando, tipo="primario", **extra):
    """Botón con foco visible, que también responde a la tecla Enter."""
    fondo, color, activo = ESTILOS_BOTON[tipo]
    opciones = dict(
        text=texto,
        command=comando,
        bg=fondo,
        fg=color,
        activebackground=activo,
        activeforeground=color,
        relief="flat",
        bd=0,
        cursor="hand2",
        font=fuente(10, "bold"),
        padx=14,
        pady=7,
        takefocus=1,
        highlightthickness=2,
        highlightbackground=fondo,
        highlightcolor="#000000",
    )
    opciones.update(extra)
    b = tk.Button(padre, **opciones)
    b.bind("<Return>", lambda evento, b=b: b.invoke())
    return b


def etiqueta_ajustable(padre, margen=24, **opciones):
    """Etiqueta cuyo texto se acomoda al ancho disponible."""
    etiqueta = tk.Label(padre, justify="left", anchor="w", **opciones)
    etiqueta.bind(
        "<Configure>",
        lambda evento: etiqueta.configure(
            wraplength=max(evento.width - margen, 120)
        )
    )
    return etiqueta


def limpiar_contenido():
    area_desplazable[0] = None
    for elemento in contenido.winfo_children():
        elemento.destroy()


def crear_titulo(titulo, descripcion):
    encabezado = tk.Frame(contenido, bg=FONDO)
    encabezado.pack(fill="x", padx=30, pady=(22, 10))

    tk.Label(
        encabezado, text=titulo, font=fuente(22, "bold"),
        bg=FONDO, fg=TEXTO
    ).pack(anchor="w")

    etiqueta_ajustable(
        encabezado, text=descripcion, font=fuente(10),
        bg=FONDO, fg=TEXTO_SUAVE
    ).pack(fill="x", pady=(4, 0))


def crear_tabla(columnas, filas, anchos=None, padre=None):
    """Tabla con barra de desplazamiento. Devuelve el Treeview."""
    padre = padre or contenido
    marco = tk.Frame(
        padre, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    marco.pack(fill="both", expand=True, padx=30, pady=(0, 18))

    tabla = ttk.Treeview(marco, columns=columnas, show="headings")
    barra = ttk.Scrollbar(marco, orient="vertical", command=tabla.yview)
    tabla.configure(yscrollcommand=barra.set)

    for k, columna in enumerate(columnas):
        tabla.heading(columna, text=columna)
        ancho = anchos[k] if anchos else 130
        tabla.column(columna, width=ancho, anchor="w")

    for fila in filas:
        tabla.insert("", "end", values=fila)

    barra.pack(side="right", fill="y", padx=(0, 6), pady=10)
    tabla.pack(side="left", fill="both", expand=True, padx=(10, 0), pady=10)

    tabla.tag_configure("reprobada", background=ROJO_FONDO, foreground="#7F1D1D")
    tabla.tag_configure("aprobada", background=VERDE_FONDO, foreground="#14532D")
    tabla.tag_configure("disponible", background="#DBEAFE", foreground="#1E3A8A")
    tabla.tag_configure("movida", background=AMBAR_FONDO, foreground="#78350F")
    return tabla


def crear_tarjeta(padre, titulo, valor, descripcion, color=None):
    color = color or AZUL
    tarjeta = tk.Frame(
        padre, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    tk.Label(
        tarjeta, text=titulo, bg=BLANCO, fg=TEXTO_SUAVE,
        font=fuente(10, "bold")
    ).pack(anchor="w", padx=15, pady=(14, 2))
    tk.Label(
        tarjeta, text=valor, bg=BLANCO, fg=color, font=fuente(26, "bold")
    ).pack(anchor="w", padx=15)
    tk.Label(
        tarjeta, text=descripcion, bg=BLANCO, fg=TEXTO, font=fuente(9)
    ).pack(anchor="w", padx=15, pady=(2, 14))
    return tarjeta


def crear_leyenda(padre, claves, nota=None):
    marco = tk.Frame(padre, bg=FONDO)
    marco.pack(fill="x", padx=30, pady=(0, 6))
    tk.Label(
        marco, text="Leyenda:", bg=FONDO, fg=TEXTO, font=fuente(10, "bold")
    ).pack(side="left", padx=(0, 8))

    for clave in claves:
        paleta = PALETA[clave]
        tk.Label(
            marco, text=paleta["etiqueta"], bg=paleta["fondo"],
            fg=paleta["texto"], font=fuente(9, "bold"), padx=8, pady=3,
            bd=0, highlightthickness=2, highlightbackground=paleta["borde"]
        ).pack(side="left", padx=3)

    if nota:
        tk.Label(
            marco, text=nota, bg=FONDO, fg=TEXTO_SUAVE, font=fuente(9)
        ).pack(side="left", padx=10)
    return marco


def sin_datos():
    """Si C no cargó la malla, muestra un aviso y devuelve True."""
    if hay_biblioteca():
        return False
    panel = tk.Frame(contenido, bg=BLANCO)
    panel.pack(fill="x", padx=30, pady=10)
    etiqueta_ajustable(
        panel,
        text=("No se cargaron las materias desde el programa en C.\n\n"
              "Compila la biblioteca con compilar_dll.bat (carpeta gui) y "
              "vuelve a abrir el programa."),
        bg=BLANCO, fg=ROJO, font=fuente(11)
    ).pack(fill="x", padx=20, pady=20)
    return True


# ======================================================================
# MAPA CURRICULAR
# ======================================================================

class MapaCurricular:
    """Dibuja las materias en columnas (una por periodo), como un mapa
    curricular. Cada materia es un botón: se puede usar con el ratón o con
    el teclado (Tab para moverse, Enter o Espacio para elegir).

    columnas : lista de (título, resumen, [índices de materias])
    modo     : "estado" (colores por aprobada/reprobada/disponible/bloqueada)
               o "plan" (colores por materia movida o sin cambio)
    rangos   : {índice: posición en el orden}, para mostrar "1.", "2.", ...
    movidas  : en modo "plan", índices de las materias que cambiaron de periodo
    """

    ANCHO_CARACTERES = 20

    def __init__(self, padre, columnas, modo="estado", rangos=None,
                 movidas=None, al_seleccionar=None):
        self.columnas = columnas
        self.modo = modo
        self.rangos = rangos or {}
        self.movidas = movidas or set()
        self.al_seleccionar = al_seleccionar
        self.seleccion = None
        self.roles = {}
        self.coincidencias = None
        self.disponibles = set()
        self.tarjetas = {}

        marco = tk.Frame(padre, bg=FONDO)
        marco.pack(fill="both", expand=True)

        self.canvas = tk.Canvas(marco, bg=FONDO, highlightthickness=0)
        barra_v = ttk.Scrollbar(marco, orient="vertical",
                                command=self.canvas.yview)
        barra_h = ttk.Scrollbar(marco, orient="horizontal",
                                command=self.canvas.xview)
        self.canvas.configure(
            yscrollcommand=barra_v.set, xscrollcommand=barra_h.set
        )
        barra_h.pack(side="bottom", fill="x")
        barra_v.pack(side="right", fill="y")
        self.canvas.pack(side="left", fill="both", expand=True)

        self.interior = tk.Frame(self.canvas, bg=FONDO)
        self.canvas.create_window((0, 0), window=self.interior, anchor="nw")
        self.interior.bind(
            "<Configure>",
            lambda evento: self.canvas.configure(
                scrollregion=self.canvas.bbox("all")
            )
        )
        area_desplazable[0] = self.canvas

        self._construir()
        self.refrescar()

    # ---- construcción -------------------------------------------------

    def _construir(self):
        ancho_px = tkfont.Font(font=fuente(10)).measure("0") * self.ANCHO_CARACTERES

        for c, (titulo, resumen, indices) in enumerate(self.columnas):
            columna = tk.Frame(self.interior, bg=FONDO)
            columna.grid(row=0, column=c, sticky="n", padx=6, pady=8)

            cabecera = tk.Frame(columna, bg=AZUL_OSCURO)
            cabecera.pack(fill="x")
            tk.Label(
                cabecera, text=titulo, bg=AZUL_OSCURO, fg=BLANCO,
                font=fuente(11, "bold")
            ).pack(padx=8, pady=(8, 0))
            tk.Label(
                cabecera, text=resumen, bg=AZUL_OSCURO, fg="#BFDBFE",
                font=fuente(9)
            ).pack(padx=8, pady=(0, 8))

            for i in indices:
                self.tarjetas[i] = self._crear_tarjeta(columna, i, ancho_px)

    def _crear_tarjeta(self, padre, i, ancho_px):
        marco = tk.Frame(padre, bg=BORDE)
        marco.pack(fill="x", pady=4)

        b = tk.Button(
            marco,
            text="",
            justify="left",
            anchor="w",
            relief="flat",
            bd=0,
            padx=8,
            pady=6,
            cursor="hand2",
            width=self.ANCHO_CARACTERES,
            wraplength=ancho_px - 16,
            font=fuente(10),
            takefocus=1,
            highlightthickness=2,
            highlightcolor="#000000",
            command=lambda i=i: self.seleccionar(i)
        )
        b.pack(fill="both", expand=True, padx=2, pady=2)
        b.bind("<Return>", lambda evento, b=b: b.invoke())
        return {"marco": marco, "boton": b}

    # ---- apariencia ---------------------------------------------------

    def _visual(self, i):
        if self.modo == "plan":
            return "movida" if i in self.movidas else "normal"
        return estado_visual(i, self.disponibles)

    def _texto(self, i, visual):
        nombre, creditos, semestre, dificultad, _ = materias[i]
        rol = self.roles.get(i)
        marca = "↑ " if rol == "requisito" else "↓ " if rol == "desbloquea" else ""
        rango = f"{self.rangos[i]}. " if i in self.rangos else ""

        if self.modo == "plan":
            if visual == "movida":
                tercera = f"↪ Plan original: sem. {semestre}"
            else:
                tercera = "= Plan original"
        else:
            tercera = PALETA[visual]["etiqueta"]

        return f"{rango}{marca}{nombre}\n{creditos} cr · Dif. {dificultad}\n{tercera}"

    def _pintar(self, i):
        tarjeta = self.tarjetas[i]
        visual = self._visual(i)
        if self.coincidencias is not None and i not in self.coincidencias \
                and self.seleccion != i:
            paleta = PALETA["atenuada"]
        else:
            paleta = PALETA[visual]

        borde = paleta["borde"]
        grosor = 4 if visual == "reprobada" else 2
        rol = self.roles.get(i)

        if self.seleccion == i:
            borde, grosor = COLOR_SELECCION, 5
        elif rol == "requisito":
            borde, grosor = COLOR_REQUISITO, 4
        elif rol == "desbloquea":
            borde, grosor = COLOR_DESBLOQUEA, 4

        tarjeta["marco"].configure(bg=borde)
        tarjeta["boton"].pack_configure(padx=grosor, pady=grosor)
        tarjeta["boton"].configure(
            text=self._texto(i, visual),
            bg=paleta["fondo"],
            fg=paleta["texto"],
            activebackground=paleta["fondo"],
            activeforeground=paleta["texto"],
            highlightbackground=paleta["fondo"]
        )

    # ---- acciones -----------------------------------------------------

    def refrescar(self):
        """Vuelve a pintar todo (por ejemplo, cuando cambia el historial)."""
        if self.modo == "estado":
            self.disponibles = set(elegibles_c())
        self.roles = {}
        if self.seleccion is not None:
            for j in descendientes(self.seleccion):
                self.roles[j] = "desbloquea"
            for j in ancestros(self.seleccion):
                self.roles[j] = "requisito"
        for i in self.tarjetas:
            self._pintar(i)

    def seleccionar(self, i, notificar=True):
        self.seleccion = None if self.seleccion == i else i
        self.refrescar()
        if notificar and self.al_seleccionar is not None:
            self.al_seleccionar(self.seleccion)

    def resaltar(self, coincidencias):
        """Atenúa las materias que no están en 'coincidencias' (None = todas)."""
        self.coincidencias = coincidencias
        for i in self.tarjetas:
            self._pintar(i)


class PanelDetalle:
    """Panel inferior con los datos de la materia elegida. Si es editable,
    permite marcarla como aprobada, reprobada o pendiente."""

    def __init__(self, padre, editable, al_cambiar=None):
        self.editable = editable
        self.al_cambiar = al_cambiar
        self.indice = None
        self.botones = {}

        self.marco = tk.Frame(
            padre, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
        )
        self.marco.pack(side="bottom", fill="x", padx=30, pady=(0, 16))

        # Primero los botones (derecha): así nunca se recortan en ventanas
        # angostas; el texto de la izquierda ocupa el espacio que sobre.
        if editable:
            derecha = tk.Frame(self.marco, bg=BLANCO)
            derecha.pack(side="right", padx=16, pady=10)
            tk.Label(
                derecha, text="Marcar como:", bg=BLANCO, fg=TEXTO,
                font=fuente(10, "bold")
            ).pack(anchor="w", pady=(0, 4))
            fila = tk.Frame(derecha, bg=BLANCO)
            fila.pack()
            for clave, texto, tipo in (
                ("aprobada", "✓ Aprobada", "aprobada"),
                ("reprobada", "✗ Reprobada", "reprobada"),
                ("pendiente", "Pendiente", "secundario"),
            ):
                b = boton(fila, texto, lambda c=clave: self.marcar(c), tipo)
                b.pack(side="left", padx=4)
                self.botones[clave] = b

        izquierda = tk.Frame(self.marco, bg=BLANCO)
        izquierda.pack(side="left", fill="both", expand=True, padx=16, pady=10)

        self.titulo = tk.Label(
            izquierda, bg=BLANCO, fg=TEXTO, anchor="w", font=fuente(13, "bold")
        )
        self.titulo.pack(fill="x")
        self.datos = etiqueta_ajustable(
            izquierda, bg=BLANCO, fg=TEXTO_SUAVE, font=fuente(10)
        )
        self.datos.pack(fill="x")
        self.aviso = etiqueta_ajustable(
            izquierda, bg=BLANCO, fg=AMBAR, font=fuente(10, "bold")
        )
        self.aviso.pack(fill="x")

        self.mostrar(None)

    def mostrar(self, i):
        self.indice = i

        if i is None:
            self.titulo.config(text="Ninguna materia seleccionada")
            texto = ("Haz clic en una materia del mapa para ver sus datos y su "
                     "ruta de prerrequisitos (↑ lo que debes aprobar antes, "
                     "↓ lo que se desbloquea con ella)")
            texto += (" y marcarla como aprobada o reprobada."
                      if self.editable else ".")
            self.datos.config(text=texto)
            self.aviso.config(text="")
            for b in self.botones.values():
                b.config(state="disabled")
            return

        nombre, creditos, semestre, dificultad, prerrequisitos = materias[i]
        dependientes = sorted(descendientes(i))
        texto = (
            f"Semestre {semestre} · {creditos} créditos · "
            f"Dificultad {dificultad}\n"
            f"Prerrequisitos: {nombres_prerrequisitos(prerrequisitos)}\n"
            f"Desbloquea: {len(dependientes)} materia(s)"
        )
        if dependientes:
            texto += ": " + ", ".join(materias[j][0] for j in dependientes[:5])
            if len(dependientes) > 5:
                texto += "…"

        visual = estado_visual(i, set(elegibles_c()))
        texto += f"\nEstado: {PALETA[visual]['etiqueta']}"

        self.titulo.config(text=nombre)
        self.datos.config(text=texto)

        faltan = prerrequisitos_sin_aprobar(i)
        if estado[i] != "pendiente" and faltan:
            self.aviso.config(
                text="Aviso: falta aprobar " + ", ".join(faltan) + "."
            )
        else:
            self.aviso.config(text="")

        for clave, b in self.botones.items():
            activo = estado[i] == clave
            b.config(
                state="normal",
                relief="sunken" if activo else "flat",
                bd=3 if activo else 0
            )

    def marcar(self, nuevo):
        if self.indice is None:
            return
        cambiar_estado(self.indice, nuevo)
        self.mostrar(self.indice)
        if self.al_cambiar is not None:
            self.al_cambiar()


# ======================================================================
# PANTALLA DE INICIO
# ======================================================================

def mostrar_inicio():
    limpiar_contenido()
    crear_titulo(
        "Panel principal",
        "Resumen de la malla curricular y de tu avance académico."
    )

    total = len(materias)
    aprobadas = cuenta_estado("aprobada")
    reprobadas = cuenta_estado("reprobada")
    pendientes = total - aprobadas - reprobadas
    creditos_total = sum(m[1] for m in materias)
    creditos_aprobados = sum(
        materias[i][1] for i in range(total) if estado[i] == "aprobada"
    )
    porcentaje = (100 * creditos_aprobados / creditos_total) if creditos_total else 0

    tarjetas = tk.Frame(contenido, bg=FONDO)
    tarjetas.pack(fill="x", padx=30, pady=6)
    for titulo, valor, descripcion, color in (
        ("Materias", str(total), "En la malla curricular", AZUL),
        ("Aprobadas", str(aprobadas), f"{creditos_aprobados} créditos", VERDE),
        ("Reprobadas", str(reprobadas), "Tienen prioridad al ordenar", ROJO),
        ("Pendientes", str(pendientes), "Por cursar", TEXTO),
    ):
        crear_tarjeta(tarjetas, titulo, valor, descripcion, color).pack(
            side="left", fill="both", expand=True, padx=(0, 12)
        )

    # Barra de avance
    panel_avance = tk.Frame(
        contenido, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    panel_avance.pack(fill="x", padx=30, pady=(10, 6))
    tk.Label(
        panel_avance,
        text=(f"Avance de créditos: {porcentaje:.0f} % "
              f"({creditos_aprobados} de {creditos_total})"),
        bg=BLANCO, fg=TEXTO, font=fuente(11, "bold")
    ).pack(anchor="w", padx=18, pady=(14, 6))
    ttk.Progressbar(
        panel_avance, orient="horizontal", mode="determinate",
        maximum=100, value=porcentaje,
        style="Avance.Horizontal.TProgressbar"
    ).pack(fill="x", padx=18, pady=(0, 16))

    # Accesos rápidos y reprobadas
    fila = tk.Frame(contenido, bg=FONDO)
    fila.pack(fill="both", expand=True, padx=30, pady=(6, 10))

    panel_accesos = tk.Frame(
        fila, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    panel_accesos.pack(side="left", fill="both", expand=True, padx=(0, 8))
    tk.Label(
        panel_accesos, text="Accesos rápidos", bg=BLANCO, fg=TEXTO,
        font=fuente(14, "bold")
    ).pack(anchor="w", padx=18, pady=(16, 8))
    for texto, funcion in (
        ("Ver el mapa curricular y marcar mi avance", mostrar_malla),
        ("Ordenar materias (reprobadas primero)", mostrar_ordenamiento),
        ("Consultar materias elegibles", mostrar_elegibles),
        ("Comparar cargas académicas", mostrar_carga),
        ("Ver la planeación por semestre", mostrar_planeacion),
    ):
        b = boton(
            panel_accesos, texto, lambda f=funcion: navegar(f), "claro",
            anchor="w"
        )
        b.pack(fill="x", padx=18, pady=4)

    panel_reprobadas = tk.Frame(
        fila, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    panel_reprobadas.pack(side="left", fill="both", expand=True, padx=(8, 0))
    tk.Label(
        panel_reprobadas, text="Materias reprobadas por recursar", bg=BLANCO,
        fg=ROJO, font=fuente(14, "bold")
    ).pack(anchor="w", padx=18, pady=(16, 8))

    indices = [i for i in range(total) if estado[i] == "reprobada"]
    if not indices:
        etiqueta_ajustable(
            panel_reprobadas,
            text=("No hay materias reprobadas registradas. Puedes marcarlas "
                  "en la pantalla «Malla curricular»."),
            bg=BLANCO, fg=TEXTO_SUAVE, font=fuente(10)
        ).pack(fill="x", padx=18, pady=(0, 16))
    else:
        for i in indices[:8]:
            tk.Label(
                panel_reprobadas,
                text=f"✗ {materias[i][0]}  (semestre {materias[i][2]}, "
                     f"{materias[i][1]} créditos)",
                bg=ROJO_FONDO, fg="#7F1D1D", anchor="w", font=fuente(10),
                padx=10, pady=4
            ).pack(fill="x", padx=18, pady=2)
        if len(indices) > 8:
            tk.Label(
                panel_reprobadas, text=f"… y {len(indices) - 8} más",
                bg=BLANCO, fg=TEXTO_SUAVE, font=fuente(10)
            ).pack(anchor="w", padx=18, pady=4)

    tk.Label(
        contenido,
        text="Datos cargados desde cursos.txt con el código en C. "
             "Tu historial se guarda en historial.json.",
        bg=FONDO, fg=TEXTO_SUAVE, font=fuente(9)
    ).pack(anchor="w", padx=30, pady=(0, 10))


# ======================================================================
# MALLA CURRICULAR (mapa editable)
# ======================================================================

def columnas_por_semestre(orden=None):
    """Columnas del mapa: una por semestre del plan. Si se da un orden,
    dentro de cada columna las materias siguen ese orden."""
    orden = orden if orden is not None else list(range(len(materias)))
    columnas = []
    for semestre in sorted({m[2] for m in materias}):
        indices = [i for i in orden if materias[i][2] == semestre]
        columnas.append((f"Semestre {semestre}", resumen_columna(indices), indices))
    return columnas


def mostrar_malla():
    limpiar_contenido()
    crear_titulo(
        "Malla curricular",
        "Mapa curricular por semestre. Haz clic en una materia para ver su "
        "ruta de prerrequisitos y marcarla como aprobada o reprobada."
    )
    if sin_datos():
        return

    zona_avisos = tk.Frame(contenido, bg=FONDO)
    zona_avisos.pack(fill="x", padx=30)

    # ---- herramientas -------------------------------------------------
    barra = tk.Frame(
        contenido, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    barra.pack(fill="x", padx=30, pady=(0, 8))

    busqueda = tk.StringVar()
    tk.Label(
        barra, text="Buscar materia:", bg=BLANCO, fg=TEXTO,
        font=fuente(10, "bold")
    ).pack(side="left", padx=(14, 6), pady=12)
    entrada = ttk.Entry(barra, textvariable=busqueda, width=24)
    entrada.pack(side="left", padx=4, pady=12)
    conteo = tk.Label(barra, text="", bg=BLANCO, fg=TEXTO_SUAVE,
                      font=fuente(10))
    conteo.pack(side="left", padx=8)

    semestres = sorted({str(m[2]) for m in materias}, key=int)
    semestre_aprobado = tk.StringVar(value=semestres[0] if semestres else "")

    boton(
        barra, "Limpiar historial", lambda: limpiar_historial(), "secundario"
    ).pack(side="right", padx=(4, 14), pady=8)
    boton(
        barra, "Aplicar", lambda: aprobar_hasta(), "claro"
    ).pack(side="right", padx=4, pady=8)
    ttk.Combobox(
        barra, textvariable=semestre_aprobado, values=semestres,
        state="readonly", width=4
    ).pack(side="right", padx=4)
    tk.Label(
        barra, text="Marcar como aprobadas hasta el semestre:", bg=BLANCO,
        fg=TEXTO, font=fuente(10, "bold")
    ).pack(side="right", padx=(14, 6))

    crear_leyenda(
        contenido, ["aprobada", "reprobada", "disponible", "bloqueada"],
        nota="Al elegir una materia: ↑ requisito · ↓ se desbloquea con ella"
    )

    # ---- panel de detalle (abajo) y mapa -------------------------------
    estado_pantalla = {}

    def al_cambiar():
        estado_pantalla["mapa"].refrescar()
        actualizar_avisos()

    panel = PanelDetalle(contenido, editable=True, al_cambiar=al_cambiar)

    cuerpo = tk.Frame(contenido, bg=FONDO)
    cuerpo.pack(fill="both", expand=True, padx=30, pady=(0, 8))
    mapa = MapaCurricular(
        cuerpo, columnas_por_semestre(), modo="estado",
        al_seleccionar=panel.mostrar
    )
    estado_pantalla["mapa"] = mapa

    # ---- comportamiento ------------------------------------------------

    def actualizar_avisos():
        for elemento in zona_avisos.winfo_children():
            elemento.destroy()
        avisos = incoherencias()
        if not avisos:
            return
        detalle = "\n".join(avisos[:4])
        if len(avisos) > 4:
            detalle += f"\n… y {len(avisos) - 4} aviso(s) más"
        etiqueta_ajustable(
            zona_avisos, text="⚠ Revisa tu historial:\n" + detalle,
            bg=AMBAR_FONDO, fg=AMBAR, font=fuente(10, "bold"),
            padx=12, pady=8
        ).pack(fill="x", pady=(0, 8))

    def aplicar_busqueda(*_):
        texto = normalizar(busqueda.get().strip())
        if not texto:
            mapa.resaltar(None)
            conteo.config(text="")
            return
        coincide = {
            i for i, m in enumerate(materias) if texto in normalizar(m[0])
        }
        mapa.resaltar(coincide)
        conteo.config(text=f"{len(coincide)} coincidencia(s)")

    def aprobar_hasta():
        try:
            limite = int(semestre_aprobado.get())
        except ValueError:
            return
        for i, materia in enumerate(materias):
            if materia[2] <= limite and estado[i] != "reprobada":
                estado[i] = "aprobada"
        guardar_historial()
        mapa.refrescar()
        panel.mostrar(mapa.seleccion)
        actualizar_avisos()

    def limpiar_historial():
        if not messagebox.askyesno(
            "Limpiar historial",
            "¿Quitar todas las marcas de aprobada y reprobada?"
        ):
            return
        for i in range(len(estado)):
            estado[i] = "pendiente"
        guardar_historial()
        mapa.refrescar()
        panel.mostrar(mapa.seleccion)
        actualizar_avisos()

    busqueda.trace_add("write", aplicar_busqueda)
    actualizar_avisos()


# ======================================================================
# ORDENAR MATERIAS (mapa curricular + prioridad a reprobadas)
# ======================================================================

def mostrar_ordenamiento():
    limpiar_contenido()
    crear_titulo(
        "Ordenar materias",
        "Elige cómo ordenar. Las materias reprobadas pueden ir primero y el "
        "resultado se muestra como mapa curricular: cada número indica la "
        "posición de la materia en el orden."
    )
    if sin_datos():
        return

    # ---- opciones -------------------------------------------------------
    barra = tk.Frame(
        contenido, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    barra.pack(fill="x", padx=30, pady=(0, 8))

    tk.Label(
        barra, text="Ordenar por:", bg=BLANCO, fg=TEXTO, font=fuente(10, "bold")
    ).pack(side="left", padx=(14, 6), pady=12)
    criterio = ttk.Combobox(
        barra, values=list(CAMPOS_ORDENAMIENTO), state="readonly", width=12
    )
    criterio.set("Dificultad")
    criterio.pack(side="left", padx=4)

    sentido = ttk.Combobox(
        barra, values=["Menor a mayor", "Mayor a menor"], state="readonly",
        width=14
    )
    sentido.set("Mayor a menor")
    sentido.pack(side="left", padx=4)

    prioridad = tk.BooleanVar(value=True)
    tk.Checkbutton(
        barra, text="Reprobadas primero", variable=prioridad,
        command=lambda: ordenar(), bg=BLANCO, fg=TEXTO,
        activebackground=BLANCO, selectcolor=BLANCO, font=fuente(10, "bold"),
        takefocus=1
    ).pack(side="left", padx=(14, 4))

    boton(barra, "Ordenar", lambda: ordenar(), "primario").pack(
        side="left", padx=10, pady=8
    )

    vista = tk.StringVar(value="mapa")
    tk.Radiobutton(
        barra, text="Lista", variable=vista, value="lista",
        command=lambda: dibujar(), bg=BLANCO, fg=TEXTO,
        activebackground=BLANCO, selectcolor=BLANCO, font=fuente(10),
        takefocus=1
    ).pack(side="right", padx=(4, 14))
    tk.Radiobutton(
        barra, text="Mapa curricular", variable=vista, value="mapa",
        command=lambda: dibujar(), bg=BLANCO, fg=TEXTO,
        activebackground=BLANCO, selectcolor=BLANCO, font=fuente(10),
        takefocus=1
    ).pack(side="right", padx=4)
    tk.Label(
        barra, text="Vista:", bg=BLANCO, fg=TEXTO, font=fuente(10, "bold")
    ).pack(side="right", padx=(10, 2))

    resumen = etiqueta_ajustable(
        contenido, text="", bg=FONDO, fg=TEXTO, font=fuente(10, "bold")
    )
    resumen.pack(fill="x", padx=30, pady=(0, 4))

    crear_leyenda(
        contenido, ["reprobada", "aprobada", "disponible", "bloqueada"],
        nota="Las reprobadas tienen el borde grueso y los números más bajos"
    )

    # ---- panel de detalle, y cuerpo con el mapa -----------------------
    seleccion = [None]
    ultimo = {"orden": [], "mapa": None}

    def al_seleccionar(i):
        seleccion[0] = i
        panel.mostrar(i)

    panel = PanelDetalle(contenido, editable=True, al_cambiar=lambda: ordenar())

    cuerpo = tk.Frame(contenido, bg=FONDO)
    cuerpo.pack(fill="both", expand=True, padx=30, pady=(0, 8))

    # ---- comportamiento ------------------------------------------------

    def dibujar():
        for elemento in cuerpo.winfo_children():
            elemento.destroy()
        area_desplazable[0] = None
        orden = ultimo["orden"]
        rangos = {indice: k + 1 for k, indice in enumerate(orden)}

        if vista.get() == "mapa":
            mapa = MapaCurricular(
                cuerpo, columnas_por_semestre(orden), modo="estado",
                rangos=rangos, al_seleccionar=al_seleccionar
            )
            ultimo["mapa"] = mapa
            if seleccion[0] is not None:
                mapa.seleccionar(seleccion[0], notificar=False)
        else:
            ultimo["mapa"] = None
            disponibles = set(elegibles_c())
            filas, etiquetas = [], []
            for posicion, i in enumerate(orden, 1):
                nombre, creditos, semestre, dificultad, _ = materias[i]
                visual = estado_visual(i, disponibles)
                filas.append((
                    posicion, nombre, semestre, creditos, dificultad,
                    PALETA[visual]["etiqueta"]
                ))
                etiquetas.append(visual)
            tabla = crear_tabla(
                ("Orden", "Materia", "Semestre", "Créditos", "Dificultad",
                 "Estado"),
                [], anchos=(70, 330, 90, 90, 100, 160), padre=cuerpo
            )
            for fila, visual in zip(filas, etiquetas):
                tabla.insert(
                    "", "end", values=fila,
                    tags=(visual,) if visual in ("reprobada", "aprobada",
                                                 "disponible") else ()
                )

    def ordenar():
        campo = CAMPOS_ORDENAMIENTO[criterio.get()]
        descendente = sentido.get() == "Mayor a menor"
        con_prioridad = prioridad.get()

        try:
            orden, motor = ordenar_materias_c(campo, descendente, con_prioridad)
        except RuntimeError as error:
            messagebox.showerror("Error", str(error))
            return

        ultimo["orden"] = orden
        reprobadas = cuenta_estado("reprobada")
        texto = "Orden aplicado: "
        if con_prioridad:
            texto += "1) reprobadas primero, 2) "
        texto += f"{criterio.get()} ({sentido.get().lower()}). "
        texto += f"{reprobadas} materia(s) reprobada(s). "
        if con_prioridad and motor == "Python":
            texto += ("Nota: cursos.dll es una versión anterior; la prioridad "
                      "se aplicó en Python. Ejecuta compilar_dll.bat para "
                      "usar la versión en C.")
            resumen.config(text=texto, fg=AMBAR)
        else:
            texto += "Ordenado con quicksort en C."
            resumen.config(text=texto, fg=TEXTO)
        dibujar()

    criterio.bind("<<ComboboxSelected>>", lambda evento: ordenar())
    sentido.bind("<<ComboboxSelected>>", lambda evento: ordenar())
    ordenar()


# ======================================================================
# MATERIAS ELEGIBLES
# ======================================================================

def mostrar_elegibles():
    limpiar_contenido()
    crear_titulo(
        "Materias elegibles",
        "Materias que puedes cursar según tu historial. Las reprobadas "
        "aparecen primero porque tienen prioridad."
    )
    if sin_datos():
        return

    avisos = incoherencias()
    if avisos:
        detalle = "\n".join(avisos[:4])
        if len(avisos) > 4:
            detalle += f"\n… y {len(avisos) - 4} aviso(s) más"
        etiqueta_ajustable(
            contenido, text="⚠ Revisa tu historial:\n" + detalle,
            bg=AMBAR_FONDO, fg=AMBAR, font=fuente(10, "bold"), padx=12, pady=8
        ).pack(fill="x", padx=30, pady=(0, 8))

    barra = tk.Frame(
        contenido, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    barra.pack(fill="x", padx=30, pady=(0, 8))

    tk.Label(
        barra, text="Mostrar:", bg=BLANCO, fg=TEXTO, font=fuente(10, "bold")
    ).pack(side="left", padx=(14, 6), pady=12)
    filtro = ttk.Combobox(
        barra, state="readonly", width=24,
        values=["Todas las pendientes", "Solo disponibles", "Solo reprobadas"]
    )
    filtro.set("Todas las pendientes")
    filtro.pack(side="left", padx=4)

    resumen = tk.Label(barra, text="", bg=BLANCO, fg=TEXTO_SUAVE,
                       font=fuente(10))
    resumen.pack(side="left", padx=14)

    boton(
        barra, "Editar mi historial en la malla",
        lambda: navegar(mostrar_malla), "claro"
    ).pack(side="right", padx=14, pady=8)

    crear_leyenda(contenido, ["reprobada", "disponible", "bloqueada"])

    tabla = crear_tabla(
        ("Prioridad", "Materia", "Semestre", "Créditos", "Prerrequisitos",
         "Estado"),
        [], anchos=(90, 300, 90, 90, 300, 330)
    )

    def llenar(*_):
        for fila in tabla.get_children():
            tabla.delete(fila)

        disponibles = set(elegibles_c())
        # Quicksort en C: por semestre, con las reprobadas primero
        orden, _motor = ordenar_materias_c(2, False, True)
        elegido = filtro.get()
        mostradas = 0

        for i in orden:
            if estado[i] == "aprobada":
                continue
            nombre, creditos, semestre, _dif, prerrequisitos = materias[i]

            if estado[i] == "reprobada":
                if i in disponibles:
                    texto, etiqueta, prioridad = (
                        "✗ Reprobada: volver a cursar", "reprobada", "Alta"
                    )
                else:
                    faltan = ", ".join(prerrequisitos_sin_aprobar(i))
                    texto, etiqueta, prioridad = (
                        f"✗ Reprobada (falta: {faltan})", "reprobada", "Alta"
                    )
            elif i in disponibles:
                texto, etiqueta, prioridad = "● Disponible", "disponible", "Normal"
            else:
                faltan = ", ".join(prerrequisitos_sin_aprobar(i))
                texto, etiqueta, prioridad = (
                    f"○ Bloqueada (falta: {faltan})", "", "—"
                )

            if elegido == "Solo disponibles" and etiqueta != "disponible" \
                    and not (estado[i] == "reprobada" and i in disponibles):
                continue
            if elegido == "Solo reprobadas" and estado[i] != "reprobada":
                continue

            tabla.insert(
                "", "end",
                values=(prioridad, nombre, semestre, creditos,
                        nombres_prerrequisitos(prerrequisitos), texto),
                tags=(etiqueta,) if etiqueta else ()
            )
            mostradas += 1

        resumen.config(text=f"{mostradas} materia(s) en la lista")

    filtro.bind("<<ComboboxSelected>>", llenar)
    llenar()


# ======================================================================
# CARGA ACADÉMICA (voraz vs. backtracking)
# ======================================================================

def mostrar_carga():
    limpiar_contenido()
    crear_titulo(
        "Carga académica",
        "Compara la heurística voraz con el backtracking según el límite de "
        "créditos. Las materias reprobadas cuentan como pendientes."
    )

    panel = tk.Frame(
        contenido, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    panel.pack(fill="x", padx=30, pady=(0, 8))

    tk.Label(
        panel, text="Límite de créditos por semestre:", bg=BLANCO, fg=TEXTO,
        font=fuente(10, "bold")
    ).pack(side="left", padx=(14, 6), pady=14)

    creditos = tk.Spinbox(panel, from_=1, to=60, width=6, font=fuente(11))
    creditos.delete(0, "end")
    creditos.insert(0, "50")
    creditos.pack(side="left", padx=8)

    resumen = tk.Label(
        contenido, text="", bg=FONDO, fg=TEXTO_SUAVE, font=fuente(10)
    )
    resumen.pack(anchor="w", padx=30, pady=(0, 5))

    tabla = crear_tabla(
        ("Algoritmo", "Semestres", "Mayor carga (créditos)", "Resultado"),
        [], anchos=(330, 110, 190, 480)
    )

    def comparar():
        try:
            limite = int(creditos.get())
        except ValueError:
            messagebox.showerror(
                "Dato inválido", "El límite de créditos debe ser un número entero."
            )
            return

        if limite < 1 or limite > 60:
            messagebox.showerror(
                "Dato inválido", "El límite de créditos debe estar entre 1 y 60."
            )
            return

        if not hay_biblioteca():
            messagebox.showerror(
                "Sin datos",
                "No se cargaron las materias desde el programa en C."
            )
            return

        for fila in tabla.get_children():
            tabla.delete(fila)

        aprobadas = construir_aprobadas()
        pendientes = len(materias) - sum(aprobadas)
        resumen.config(
            text=(f"Materias pendientes: {pendientes} | "
                  f"Límite: {limite} créditos por semestre")
        )

        for texto, criterio in (
            ("Voraz: materias que más desbloquean", 0),
            ("Voraz: solo por créditos", 1),
        ):
            semestres, asignacion = plan_voraz_c(criterio, limite)
            if semestres < 0:
                messagebox.showerror(
                    "Error",
                    "No se pudo calcular el plan: la malla tiene un ciclo de "
                    "prerrequisitos."
                )
                return
            carga = max(creditos_por_semestre(asignacion).values(), default=0)
            tabla.insert(
                "", "end",
                values=(texto, semestres, carga,
                        "Heurística: no garantiza el mínimo")
            )

        fila_bt = tabla.insert(
            "", "end",
            values=("Backtracking (búsqueda exhaustiva)", "...", "...",
                    "Calculando...")
        )
        boton_comparar.config(state="disabled")

        respuesta = {}

        def trabajo():
            try:
                respuesta["bt"] = plan_backtracking_c(limite, aprobadas)
            except Exception:
                respuesta["bt"] = {"codigo": -1}

        threading.Thread(target=trabajo, daemon=True).start()

        def revisar():
            try:
                if not tabla.winfo_exists():
                    return
            except tk.TclError:
                return

            if "bt" not in respuesta:
                ventana.after(150, revisar)
                return

            resultado = respuesta["bt"]
            codigo = resultado["codigo"]
            nombre = "Backtracking (búsqueda exhaustiva)"

            if codigo == 0:
                carga = max(
                    creditos_por_semestre(resultado["asignacion"]).values(),
                    default=0
                )
                if resultado["optimo"]:
                    nota = "Exacto: mínimo de semestres demostrado"
                else:
                    nota = ("Plan válido; mínimo no demostrado "
                            "(se llegó al límite de pasos)")
                valores = (nombre, resultado["semestres"], carga, nota)
            elif codigo == 1:
                valores = (nombre, "-", "-", "Sin plan dentro del límite de pasos")
            elif codigo == -2:
                valores = (nombre, "-", "-", "Hay más de 64 materias pendientes")
            else:
                valores = (nombre, "-", "-",
                           "Error: ciclo de prerrequisitos o memoria")

            tabla.item(fila_bt, values=valores)
            boton_comparar.config(state="normal")

        ventana.after(150, revisar)

    boton_comparar = boton(panel, "Comparar algoritmos", comparar, "primario")
    boton_comparar.pack(side="left", padx=10, pady=8)


# ======================================================================
# PLANEACIÓN SEMESTRAL (mapa del plan sugerido)
# ======================================================================

def mostrar_planeacion():
    limpiar_contenido()
    crear_titulo(
        "Planeación semestral",
        "Plan sugerido de materias por periodo, calculado con quicksort y las "
        "restricciones de prerrequisitos."
    )
    if sin_datos():
        return

    semestres, asignacion = plan_sugerido_c(
        MAX_CREDITOS_SEMESTRE, MAX_DIFICULTAD_SEMESTRE
    )
    if semestres < 0:
        messagebox.showerror(
            "Error",
            "No se pudo calcular el plan. Revisa que las materias se hayan "
            "cargado y que la malla no tenga ciclos."
        )
        return

    columnas, movidas = [], set()
    for periodo in range(1, semestres + 1):
        indices = [i for i in range(len(materias)) if asignacion[i] == periodo]
        if not indices:
            continue
        for i in indices:
            if materias[i][2] != periodo:
                movidas.add(i)
        columnas.append((f"Semestre {periodo}", resumen_columna(indices), indices))

    etiqueta_ajustable(
        contenido,
        text=(f"Máximo {MAX_CREDITOS_SEMESTRE} créditos y dificultad "
              f"{MAX_DIFICULTAD_SEMESTRE} por semestre. "
              f"{len(movidas)} materia(s) cambiaron de semestre respecto al "
              "plan original."),
        bg=FONDO, fg=AMBAR, font=fuente(10, "bold")
    ).pack(fill="x", padx=30, pady=(0, 6))

    crear_leyenda(
        contenido, ["normal", "movida"],
        nota="Al elegir una materia: ↑ requisito · ↓ se desbloquea con ella"
    )

    panel = PanelDetalle(contenido, editable=False)
    cuerpo = tk.Frame(contenido, bg=FONDO)
    cuerpo.pack(fill="both", expand=True, padx=30, pady=(0, 8))
    MapaCurricular(
        cuerpo, columnas, modo="plan", movidas=movidas,
        al_seleccionar=panel.mostrar
    )


# ======================================================================
# REPORTES
# ======================================================================

def mostrar_reportes():
    limpiar_contenido()
    crear_titulo(
        "Reportes",
        "Validación de la malla y créditos por semestre."
    )

    invalidos, ciclos = validar_malla_c()

    panel = tk.Frame(
        contenido, bg=BLANCO, highlightbackground=BORDE, highlightthickness=1
    )
    panel.pack(fill="x", padx=30, pady=(0, 10))

    tk.Label(
        panel, text="Validación de la malla", bg=BLANCO, fg=TEXTO,
        font=fuente(14, "bold")
    ).pack(anchor="w", padx=20, pady=(18, 6))

    if not hay_biblioteca():
        mensaje = "✗ No se cargaron las materias desde el programa en C."
        color = ROJO
    elif invalidos == 0 and not ciclos:
        mensaje = "✓ Sin ciclos de prerrequisitos ni índices inválidos."
        color = VERDE
    else:
        lineas = []
        if invalidos > 0:
            lineas.append(f"✗ Prerrequisitos con índice inválido: {invalidos}.")
        for ciclo in ciclos:
            nombres = [materias[i][0] for i in ciclo]
            nombres.append(nombres[0])
            lineas.append("✗ Ciclo: " + " -> ".join(nombres))
        if ciclos:
            lineas.append('(cada flecha se lee "requiere a")')
        mensaje = "\n".join(lineas)
        color = ROJO

    etiqueta_ajustable(
        panel, text=mensaje, bg=BLANCO, fg=color, font=fuente(11, "bold")
    ).pack(fill="x", padx=20, pady=(0, 18))

    filas = []
    totales = [0, 0, 0, 0, 0]
    for semestre in sorted({m[2] for m in materias}):
        indices = [i for i, m in enumerate(materias) if m[2] == semestre]
        valores = [
            len(indices),
            sum(materias[i][1] for i in indices),
            sum(materias[i][3] for i in indices),
            sum(materias[i][1] for i in indices if estado[i] == "aprobada"),
            sum(1 for i in indices if estado[i] == "reprobada"),
        ]
        filas.append((f"Semestre {semestre}", *valores))
        totales = [t + v for t, v in zip(totales, valores)]

    filas.append(("Total", *totales))

    crear_tabla(
        ("Periodo", "Materias", "Créditos", "Dificultad total",
         "Créditos aprobados", "Reprobadas"),
        filas, anchos=(160, 110, 110, 150, 170, 120)
    )


# ======================================================================
# MENÚ LATERAL Y NAVEGACIÓN
# ======================================================================

OPCIONES_MENU = (
    ("Inicio", mostrar_inicio),
    ("Malla curricular", mostrar_malla),
    ("Ordenar materias", mostrar_ordenamiento),
    ("Materias elegibles", mostrar_elegibles),
    ("Carga académica", mostrar_carga),
    ("Planeación", mostrar_planeacion),
    ("Reportes", mostrar_reportes),
)


def marcar_menu(funcion):
    """Resalta en el menú la pantalla que se está viendo."""
    for destino, b in botones_menu.items():
        activo = destino is funcion
        b.configure(
            bg=AZUL if activo else AZUL_OSCURO,
            highlightbackground=AZUL if activo else AZUL_OSCURO,
            font=fuente(11, "bold" if activo else "normal")
        )


def navegar(funcion):
    global pantalla_actual
    pantalla_actual = funcion
    marcar_menu(funcion)
    funcion()


def cambiar_escala(paso):
    """Agranda (+1) o reduce (-1) el tamaño de todo el texto."""
    global ESCALA
    posicion = ESCALAS.index(ESCALA) if ESCALA in ESCALAS else 1
    posicion = min(max(posicion + paso, 0), len(ESCALAS) - 1)
    ESCALA = ESCALAS[posicion]
    aplicar_estilos()
    construir_menu()
    if pantalla_actual is not None:
        pantalla_actual()


def construir_menu():
    for elemento in menu_lateral.winfo_children():
        elemento.destroy()
    botones_menu.clear()
    menu_lateral.configure(width=int(240 * min(ESCALA, 1.25)))

    tk.Label(
        menu_lateral, text="Proyecto", font=fuente(23, "bold"),
        bg=AZUL_OSCURO, fg=BLANCO
    ).pack(anchor="w", padx=22, pady=(28, 3))
    tk.Label(
        menu_lateral, text="PLANEACIÓN CURRICULAR", font=fuente(8, "bold"),
        bg=AZUL_OSCURO, fg="#BFDBFE"
    ).pack(anchor="w", padx=22, pady=(0, 22))

    for texto, funcion in OPCIONES_MENU:
        b = tk.Button(
            menu_lateral, text=texto, command=lambda f=funcion: navegar(f),
            font=fuente(11), bg=AZUL_OSCURO, fg=BLANCO,
            activebackground="#1E40AF", activeforeground=BLANCO,
            relief="flat", bd=0, cursor="hand2", anchor="w",
            padx=22, pady=12, takefocus=1, highlightthickness=2,
            highlightbackground=AZUL_OSCURO, highlightcolor=BLANCO
        )
        b.bind("<Return>", lambda evento, b=b: b.invoke())
        b.pack(fill="x", padx=8, pady=2)
        botones_menu[funcion] = b

    pie = tk.Frame(menu_lateral, bg=AZUL_OSCURO)
    pie.pack(side="bottom", fill="x", pady=16)
    tk.Label(
        pie, text="Tamaño del texto", font=fuente(9, "bold"),
        bg=AZUL_OSCURO, fg="#BFDBFE"
    ).pack(anchor="w", padx=22, pady=(0, 6))
    fila = tk.Frame(pie, bg=AZUL_OSCURO)
    fila.pack(anchor="w", padx=18)
    for texto, paso in (("A−", -1), ("A+", 1)):
        tk.Button(
            fila, text=texto, command=lambda p=paso: cambiar_escala(p),
            font=(FAMILIA, 12, "bold"), bg="#1E3A8A", fg=BLANCO,
            activebackground="#1E40AF", activeforeground=BLANCO,
            relief="flat", bd=0, cursor="hand2", width=4, pady=5,
            takefocus=1, highlightthickness=2,
            highlightbackground="#1E3A8A", highlightcolor=BLANCO
        ).pack(side="left", padx=4)
    tk.Label(
        pie, text="Versión de desarrollo", font=fuente(8),
        bg=AZUL_OSCURO, fg="#CBD5E1"
    ).pack(anchor="w", padx=22, pady=(12, 0))

    marcar_menu(pantalla_actual)


construir_menu()

# Si C no pudo cargar la malla, avisamos a la persona
if error_carga is not None:
    messagebox.showerror(
        "No se pudo cargar la malla",
        "El programa en C no pudo leer las materias.\n\n"
        + error_carga
        + "\n\nCompila la biblioteca con compilar_dll.bat "
          "(carpeta gui) y vuelve a abrir el programa."
    )

navegar(mostrar_inicio)

if __name__ == "__main__":
    ventana.mainloop()
