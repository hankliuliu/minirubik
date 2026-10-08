    .text
main:
    li   t0, 0
    li   t1, 40000
loop:
    addi t0, t0, 1
    bne  t0, t1, loop
    li   a7, 10
    ecall
