CC = gcc
CFLAGS = -O2 -std=c11 -Wall -Wextra -I../stage2 -I.
S2 = ../stage2

all: profile elfs

$(S2)/tables.c:
	$(MAKE) -C $(S2) tables.c

# Host profile: every version under its own name, with counters.
profile: profile.c ida_v1.c ida_v2.c ida_v3.c $(S2)/ida.c $(S2)/tables.c stats.h
	$(CC) $(CFLAGS) -DIDA_STATS -Dida_solve=ida_solve_v0 -c -o v0.o $(S2)/ida.c
	$(CC) $(CFLAGS) -DIDA_STATS -Dida_solve=ida_solve_v1 -c -o v1.o ida_v1.c
	$(CC) $(CFLAGS) -DIDA_STATS -Dida_solve=ida_solve_v2 -c -o v2.o ida_v2.c
	$(CC) $(CFLAGS) -DIDA_STATS -Dida_solve=ida_solve_v3 -c -o v3.o ida_v3.c
	$(CC) $(CFLAGS) -DIDA_STATS -o $@ profile.c v0.o v1.o v2.o v3.o $(S2)/tables.c
	rm -f v0.o v1.o v2.o v3.o

# Ripes ELFs for every version and test vector.
elfs: $(S2)/tables.c
	./build.sh

clean:
	rm -rf profile check build

.PHONY: all elfs gates clean

# Gates H1-H4 on the final C search (v3) instead of stage2/ida.c.
check: $(S2)/check.c ida_v3.c counters.c $(S2)/tables.c stats.h
	$(CC) $(CFLAGS) -fopenmp -DIDA_STATS -o $@ $(S2)/check.c ida_v3.c counters.c $(S2)/tables.c

gates: check
	./check
