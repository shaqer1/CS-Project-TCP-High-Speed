#
# This is an example Makefile for a countwords program.  This
# program uses both the scanner module and a counter module.
# Typing 'make' will create the executable file.
#

# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = g++
CFLAGS  = -g -Wall

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: prog2.o tasks_alt tasks_gbn tasks_sr

# To create the executable file count we need the object files

tasks_alt: tasks_alt.o prog2.o
	$(CC) $(CFLAGS) tasks_alt.o prog2.o -o tasks_alt 

tasks_gbn: tasks_gbn.o prog2.o
	$(CC) $(CFLAGS) tasks_gbn.o prog2.o -o tasks_gbn 

tasks_sr: tasks_sr.o prog2.o
	$(CC) $(CFLAGS) tasks_sr.o prog2.o -o tasks_sr 

# To create the object files *.o, we need the source files

tasks_alt.o:  tasks_alt.cpp prog2.h
	$(CC) $(CFLAGS) -c tasks_alt.cpp

tasks_gbn.o: tasks_gbn.cpp prog2.h 
	$(CC) $(CFLAGS) -c tasks_gbn.cpp

tasks_sr.o:  tasks_sr.cpp prog2.h 
	$(CC) $(CFLAGS) -c tasks_sr.cpp

prog2.o: prog2.cpp prog2.h 
	$(CC) $(CFLAGS) -c prog2.cpp

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) tasks_alt tasks_gbn tasks_sr *.o *~