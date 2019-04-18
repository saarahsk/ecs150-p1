all: main.o
	gcc -g -o sshell main.o

main.o: main.c
	gcc -Wall -Werror -Wimplicit-function-declaration -Wint-conversion -Wincompatible-pointer-types -Wno-deprecated -g -c -o main.o main.c

clean:
	rm -rf *.o *.dSYM sshell
