#CC=gcc
#CFLAGS=
#FMT=indent

#mysh: shell.c interpreter.c shellmemory.c
#	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c
#	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o

#style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h
#	$(FMT) $?

#clean: 
#	$(RM) mysh; $(RM) *.o; $(RM) *~

CC=gcc
CFLAGS=-pthread
FMT=indent
framesize ?= 18
varmemsize ?= 10

ifeq ($(strip $(framesize)),)
override framesize := 18
endif

ifeq ($(strip $(varmemsize)),)
override varmemsize := 10
endif

mysh: shell.c interpreter.c shellmemory.c queue.c
	$(CC) $(CFLAGS) -D fS_lines=$(framesize) -D MEM_SIZE=$(varmemsize) -c shell.c interpreter.c shellmemory.c queue.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o queue.o

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h pcb.h queue.c queue.h
	$(FMT) $?

clean:
	$(RM) mysh; $(RM) *.o; $(RM) *~
