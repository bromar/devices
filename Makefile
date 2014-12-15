## CS179F Fall 2014: Senior Design in OS
## Devices Team
## Makefile
##
## This file works with OS X and Linux
#########################################

OS := $(shell uname)

# OS X
ifeq ($(OS), Darwin)
LIBS = 
## Compiler flags
FLAGS = -std=c++11 -stdlib=libc++
endif

# Linux
ifeq ($(OS), Linux)
LIBS = 
## Compiler flags
FLAGS = -std=c++ -pthread
endif

## Compiler
CC = g++

## Global header files
INCLUDE =

## Object files and executables
MAIN_OUT = devices.out

## Requirements for each command
MAIN_REQS =

## Target source to compile for each command
MAIN_TARGETS = main.cpp devices.cpp

## Objects from source
MAIN_OBJS = main.o devices.o

all:
		$(CC) $(FLAGS) -c $(MAIN_TARGETS)
		$(CC) $(FLAGS) -o $(MAIN_OUT) $(MAIN_OBJS)

test:
		./$(MAIN_OUT)

print:
		a2ps --font-size=8pts -E C++ --line-numbers=1 -M letter $(INCLUDE) $(MAIN_TARGETS) Makefile -o printout.ps

size:
		cat $(INCLUDE) $(MAIN_TARGETS) | wc -l

clean:
		rm -r *.o
