all: sshell.o
	gcc -g -o sshell sshell.o

sshell.o: sshell.c
	gcc -Wall -Werror -Wimplicit-function-declaration -Wint-conversion -Wincompatible-pointer-types -Wno-deprecated -g -c -o sshell.o sshell.c

clean:
	rm -rf *.o *.dSYM sshell
