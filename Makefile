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
endif

# Linux
ifeq ($(OS), Linux)
LIBS = 
endif

## Compiler
CC = g++

## Compiler flags
FLAGS = -std=c++11 -stdlib=libc++

## Global header files
INCLUDE = 

## Object files and executables
MAIN_OUT = devices

## Requirements for each command
MAIN_REQS =

## Targets to compile for each command
MAIN_TARGETS = device_system.cc

all:
		$(CC) $(FLAGS) -o $(MAIN_OUT).out $(MAIN_TARGETS) $(LIBS)

test:
		./$(MAIN_OUT).out

print:
		a2ps --font-size=8pts -E C++ --line-numbers=1 -M letter $(INCLUDE) $(MAIN_TARGETS) Makefile -o printout.ps

size:
		cat $(INCLUDE) $(MAIN_TARGETS) | wc -l

clean:
		rm -r *.o
