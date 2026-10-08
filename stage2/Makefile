CC = gcc
CFLAGS = -O2 -std=c11 -Wall -Wextra

all: check

gen_tables: gen_tables.c cube.h tables.h
	$(CC) $(CFLAGS) -o $@ gen_tables.c

tables.c tables.s &: gen_tables
	./gen_tables

check: check.c ida.c ida.h tables.c tables.h cube.h
	$(CC) $(CFLAGS) -fopenmp -DIDA_STATS -o $@ check.c ida.c tables.c

explore: explore.c
	$(CC) $(CFLAGS) -o $@ explore.c

gates: check
	./check

clean:
	rm -f gen_tables check explore tables.c tables.s

.PHONY: all gates clean
