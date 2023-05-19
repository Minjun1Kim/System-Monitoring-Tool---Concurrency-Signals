#CC : compiler
CC = gcc
#compiler flags
CFLGGS = -Wall -g -std=c99 -Werror

## All: run the prog target
.PHONY: All
All: prog

## prog: link all the .o file dependencies to create the executable
prog: main.o stats_functions.o
	$(CC) $(CFLAGS) -o $@ $^

##%.o: compile all .c files to .o files
%.o: %.c a3.h
	$(CC) $(CFLAGS) -c $<

## clean : remove all object files and executable
.PHONY: clean
clean:
	rm -f *.o prog

## help: display this help message
.PHONY: help
help: Makefile
	@sed -n 's/^##//p' $<
