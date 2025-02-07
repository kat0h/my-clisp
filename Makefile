CC = gcc
all: main.o continuation.o
	$(CC) -o scm main.o continuation.o
main.o: main.c main.h continuation.o
	$(CC) -c main.c
continuation.o: continuation.c continuation.h
	$(CC) -c continuation.c
clean:
	rm -rf main.o scm
tags:
	ctags -R .
