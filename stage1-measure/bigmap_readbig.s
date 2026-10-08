    .text
main:
    li   t0, 0x10000000      # phase 1: touch 16 MiB
    li   t1, 0x11000000
fill:
    sw   t0, 0(t0)
    addi t0, t0, 4
    bne  t0, t1, fill
    li   t2, 4    # phase 2: read passes over the region
    li   t1, 0x11000000
pass:
    li   t0, 0x10000000
read:
    lw   t3, 0(t0)
    addi t0, t0, 4
    bne  t0, t1, read
    addi t2, t2, -1
    bne  t2, zero, pass
    li   a7, 10
    ecall
