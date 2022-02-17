CFLAGS = -std=c11 -pthread -Wpedantic -Wall -Wextra -Wconversion -Werror -fstack-protector-all -D_XOPEN_SOURCE=600 -D_FORTIFY_SOURCE=2 -O2 
LDFLAGS = -L. -pthread
LDLIBS = -lrt
#LDLIBS = -lfifo -lrt
EXECUTABLE = lanceur client daemon info_proc
#LIBRARY = libfifo.so

#all : $(LIBRARY) $(EXECUTABLE) clean
all : $(EXECUTABLE) clean

client : client.o kit_client.o threads.o file_synch.o
lanceur : lanceur.o log.o  kit_lanceur.o threads.o file_synch.o
daemon : daemon.o log.o
info_proc : info_proc.o

#libfifo.so : file_synch.o
#	gcc -shared $< -o $@	
#file_synch.o : file_synch.c file_synch.h
#	gcc -fPIC $(CFLAGS)  -c $<
clean : 
	$(RM) *.o $(EXECUTABLE)
uninstall: 
	$(RM) -r $(EXECUTABLE)

#export LD_LIBRARY_PATH=.
