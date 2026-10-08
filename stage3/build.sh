#!/bin/bash
# Builds the Ripes ELFs: every search version x every test vector, with
# gcc -O2 -march=rv32i -mabi=ilp32.  No libc and no libgcc are linked, so a
# leftover __mulsi3 or __divsi3 call fails the link.
set -e
cd "$(dirname "$0")"
RV=~/opt/rv/usr/bin
CFLAGS="-O2 -march=rv32i -mabi=ilp32 -ffreestanding -nostdlib -nostartfiles
        -fno-tree-loop-distribute-patterns -I../stage2 -I. -T ripes.ld"
mkdir -p build

# name:vector:distance
TESTS="solved:12345671111111:0 short:24173562322133:3
       d11:21345671111111:11 worst:54721631111111:11"
SRC_v0=../stage2/ida.c
SRC_v1=ida_v1.c
SRC_v2=ida_v2.c
SRC_v3=ida_v3.c

build() { # out search-source vector distance extra-flags
    $RV/riscv64-unknown-elf-gcc $CFLAGS -DINPUT="\"$3\"" -DEXPECT=$4 $5 \
        -o build/$1.elf start.S main.c $2 ../stage2/tables.c
}

for v in v0 v1 v2 v3; do
    src=SRC_$v
    for t in $TESTS; do
        IFS=: read -r name vec d <<< "$t"
        build ${v}_$name ${!src} $vec $d
    done
done
build v1_worst_mod3bl $SRC_v1 54721631111111 11 -DMOD3_BRANCHLESS

# Sizes: ida_solve alone, whole .text, static data.
printf '%-20s %10s %8s %12s\n' elf ida_solve .text static
for e in build/*.elf; do
    fn=$($RV/riscv64-unknown-elf-nm -S --size-sort $e | awk '$4 == "ida_solve" {print strtonum("0x" $2)}')
    $RV/riscv64-unknown-elf-size -A $e | awk -v e=$(basename $e) -v fn=$fn '
        $1 == ".text" {t = $2}
        $1 == ".rodata" || $1 == ".data" || $1 == ".bss" {s += $2}
        END {printf "%-20s %10d %8d %12d\n", e, fn, t, s}'
done
