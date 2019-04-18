output: sshell.o
	gcc -o sshell -Wall -Wimplicit-function-declaration -Wint-conversion -Wincompatible-pointer-types -Werror -Wno-deprecated sshell.o

sshell.o: sshell.c
	gcc -c sshell.c

clean:
	rm sshell *.o
