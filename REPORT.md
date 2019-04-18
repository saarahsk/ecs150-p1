The sshell assignment was implemented by using fork, exec, and wait system
calls to run the commands that users entered. The final program we created
is similar to the command prompts that are available on the linux systems in
CSIF and other linux computers. The program was implemented by using a while
loop to constantly ask the user to enter input and then parsing the user input
to understand what the user was trying to do. This was actually the hardest
part of the program because we had to work with the string and split it up.
There are some functions available like strtok that split the string but the
command that the user enters can use quotes or not use spaces around certain
parts like in input redirection. That meant that we couldn't really use strtok
or other functions like it and we had to do the command parsing ourselves.
There were a lot of issues that came up with the parsing code and it took a
really long time to implement and get it right. Even still, there might still
be some bugs in it.

After the command was parsed, we placed it in a data structure to help us use
the command later without having to reparse the command. Doing this work was
actually really helpful when we got to the harder parts of the project with
input redirection. Since everything we needed was in the data structure already
we could just access it from there directly.

Doing input redirection was done by just looking at the slides from lecture
to understand how to manage file descriptors and use the dup2 command. The
concepts from those slides allowed us to implement the input and output
redirection without any issue.

Doing the built in commands was fairly easy because we didn't have to fork. We
just handled that before the fork and avoided forking if the user entered a
built in command. One issue came up when we wanted to redirect the output of
a built in command. We didn't have a program that we could update the
standard input of since we weren't forking anymore. We had to fake that
a bit.

We tested mostly manually by trying to run different commands with the shell.
We weren't able to get the test script working unfortunately.

We used a lot of resources to finish this program. The main resources were from
stack overflow which explained how to use a lot of the system calls we made.
We also used the lecture slides a lot to reference the sample code that was
provided in them.