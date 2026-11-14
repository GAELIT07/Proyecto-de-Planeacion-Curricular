CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I../curso -I../leer_Archivo -I../quicksort -I../voraz
OBJS = u4_main.o validacion.o elegibles.o backtracking.o voraz.o quicksort.o curso.o leerArchivo.o
BIN = u4

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS)

u4_main.o: u4_main.c validacion.h elegibles.h backtracking.h
	$(CC) $(CFLAGS) $(INCLUDES) -c u4_main.c

validacion.o: validacion.c validacion.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c validacion.c

elegibles.o: elegibles.c elegibles.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c elegibles.c

backtracking.o: backtracking.c backtracking.h ../voraz/voraz.h ../quicksort/quicksort.h
	$(CC) $(CFLAGS) $(INCLUDES) -c backtracking.c

voraz.o: ../voraz/voraz.c ../voraz/voraz.h ../curso/curso.h ../quicksort/quicksort.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../voraz/voraz.c

quicksort.o: ../quicksort/quicksort.c ../quicksort/quicksort.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../quicksort/quicksort.c

leerArchivo.o: ../leer_Archivo/leerArchivo.c ../leer_Archivo/leerArchivo.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../leer_Archivo/leerArchivo.c

curso.o: ../curso/curso.c ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../curso/curso.c

run: all
	./$(BIN)

clean:
	rm -f $(OBJS) $(BIN) $(BIN).exe
