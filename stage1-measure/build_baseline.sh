#!/bin/bash
# Build the baseline solver.c as an RV32I ELF for Ripes (run inside WSL).
# Toolchain: Ubuntu 24.04 gcc-riscv64-unknown-elf 13.2 + picolibc 1.8.6,
# unpacked with `apt-get download` + `dpkg -x` into ~/opt/rv (no sudo).
set -e
cd "$(dirname "$0")"
RV=~/opt/rv/usr
PL=$RV/lib/picolibc/riscv64-unknown-elf
CC=$RV/bin/riscv64-unknown-elf-gcc
SRC=${SRC:-$HOME/minirubik}           # the fork, unmodified
INPUT=${INPUT:-21345671111111}

# Ripes treats only the section named .text as executable and stops as soon
# as the PC leaves it (ProcessorHandler::_loadProgram), and it places every
# section at its VMA. picolibc.ld puts the entry code in a separate .init
# section and loads .data from a flash copy, so derive a Ripes-friendly
# script: .init merged into .text, .data/.tdata loaded where they run, and
# the memory map and a 64 KiB stack fixed inside the script (on the command
# line, --defsym comes after -T and DEFINED() does not see it).
perl -0pe '
  s/\t\.init : \{\n(.*?)\n\t\} >flash AT>flash :text\n\n\t\.text : \{\n/\t.text : {\n$1\n/s;
  s/>ram AT>flash/>ram AT>ram/g;
' "$PL/lib/picolibc.ld" > ripes.ld
sed -i '1i __flash = 0x00000000; __flash_size = 0x00100000;\n__ram = 0x10000000; __ram_size = 0x02000000;\n__stack_size = 0x00010000;' ripes.ld
if grep -q -P '^\t\.init :' ripes.ld; then echo "patch failed: .init still separate"; exit 1; fi

# Same flags as the assignment's reference build, plus picolibc.
$CC -march=rv32i -mabi=ilp32 -O2 -std=c99 \
    --specs=$PL/picolibc.specs --picolibc-prefix=$RV -T ripes.ld \
    -I"$SRC" -DINPUT="\"$INPUT\"" \
    -o baseline_$INPUT.elf baseline_ripes.c

$RV/bin/riscv64-unknown-elf-size -A baseline_$INPUT.elf | grep -E '^\.(text|rodata|data|bss|stack|heap) '
$RV/bin/riscv64-unknown-elf-readelf -h baseline_$INPUT.elf | grep Entry
$RV/bin/riscv64-unknown-elf-objdump -d baseline_$INPUT.elf > baseline_$INPUT.dis
echo "native: $("$SRC/solver" "$INPUT")"
