    .text
main:
    li   t0, 0
    li   t1, 10000
loop:
    addi t0, t0, 1
    beq  t0, zero, never     # never taken
    bne  t0, t1, loop
never:
    li   a7, 10
    ecall
