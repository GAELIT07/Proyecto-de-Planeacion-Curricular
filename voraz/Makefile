CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I../curso -I../leer_Archivo -I../quicksort
OBJS = voraz_main.o voraz.o quicksort.o curso.o leerArchivo.o
BIN = voraz

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS)

voraz_main.o: voraz_main.c voraz.h
	$(CC) $(CFLAGS) $(INCLUDES) -c voraz_main.c

voraz.o: voraz.c voraz.h ../curso/curso.h ../quicksort/quicksort.h
	$(CC) $(CFLAGS) $(INCLUDES) -c voraz.c

quicksort.o: ../quicksort/quicksort.c ../quicksort/quicksort.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../quicksort/quicksort.c

leerArchivo.o: ../leer_Archivo/leerArchivo.c ../leer_Archivo/leerArchivo.h ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../leer_Archivo/leerArchivo.c

curso.o: ../curso/curso.c ../curso/curso.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ../curso/curso.c

run: all
	./$(BIN)

clean:
	rm -f $(OBJS) $(BIN)
