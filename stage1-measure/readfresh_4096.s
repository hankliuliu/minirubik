    .text
main:
    li   t0, 0x10000000
    li   t1, 0x10400000
loop:
    lw   t3, 0(t0)           # read memory that was never written
    addi t0, t0, 4
    bne  t0, t1, loop
    li   a7, 10
    ecall
