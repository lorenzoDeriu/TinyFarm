CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread

MAIN=farm
TEST=test

all: $(MAIN)

farm: farm.o xerrori.o

test: test.o xerrori.o

clean: 
	rm -f $(MAIN) $(TEST) *.o