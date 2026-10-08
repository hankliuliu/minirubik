    .text
main:
    li   t0, 0
    li   t1, 10000
loop:
    addi t0, t0, 1
    addi t2, t2, 1
    addi t3, t3, 1
    addi t4, t4, 1
    bne  t0, t1, loop
    li   a7, 10
    ecall
