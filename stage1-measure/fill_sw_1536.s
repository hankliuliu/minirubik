    .text
main:
    li   t0, 0x10000000      # t0 = current guest address
    li   t1, 0x10180000      # t1 = one past the last byte to write
loop:
    sw   t0, 0(t0)           # store a non-zero value to a fresh address
    addi t0, t0, 4
    bne  t0, t1, loop
    li   a7, 10              # exit
    ecall
