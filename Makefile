all: sshell.o
	gcc -o sshell -Wall  -Werror -Wno-deprecated sshell.o

sshell.o: sshell.c
	gcc -c sshell.c

clean:
	rm sshell *.o
