# Entry point for the freestanding Ripes build: main's return value becomes
# the exit code (Exit2 ecall, a7 = 93).
    .section .text.start
    .globl _start
_start:
    li   sp, 0x7ffffff0
    call main
    li   a7, 93
    ecall
1:  j    1b
