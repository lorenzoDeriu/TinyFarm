CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread

MAIN=farm

all: $(MAIN)

farm: farm.o xerrori.o

clean: 
	rm -f $(MAIN) *.o
