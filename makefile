CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread

MAIN=farm
TEST=test

all: $(MAIN) start-server start-program

farm: farm.o xerrori.o

start-server: 
	python3 collector.py &
start-program: 
	./farm z0.dat z1.dat

test: test.o xerrori.o

clean: 
	rm -f $(MAIN) $(TEST) *.o