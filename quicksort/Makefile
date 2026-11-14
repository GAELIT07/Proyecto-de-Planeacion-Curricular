CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I../curso -I../leer_Archivo -I../restricciones -I../semestre_sugerido
OBJS = quicksort.o quicksort_main.o curso.o leerArchivo.o restricciones.o semestre_sugerido.o
BIN = planeacion

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS)

quicksort.o: quicksort.c quicksort.h ../curso/curso.h ../restricciones/restricciones.h ../semestre_sugerido/semestre_sugerido.h
	$(CC) $(CFLAGS) $(INCLUDES) -c quicksort.c

quicksort_main.o: quicksort_main.c quicksort.h
	$(CC) $(CFLAGS) $(INCLUDES) -c quicksort_main.c

leerArchivo.o: ../leer_Archivo/leerArchivo.c ../leer_Archivo/leerArchivo.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../leer_Archivo/leerArchivo.c

curso.o: ../curso/curso.c ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../curso/curso.c

restricciones.o: ../restricciones/restricciones.c ../restricciones/restricciones.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../restricciones/restricciones.c

semestre_sugerido.o: ../semestre_sugerido/semestre_sugerido.c ../semestre_sugerido/semestre_sugerido.h ../curso/curso.h quicksort.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../semestre_sugerido/semestre_sugerido.c

run: all
	./$(BIN)

clean:
	rm -f $(OBJS) $(BIN)
