all: main.o
	cc -o scm main.o continuation.o
main.o: main.c continuation.o
	cc -c main.c
continuation.o: continuation.c
	cc -c continuation.c
clean:
	rm -rf main.o scm
