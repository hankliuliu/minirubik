    .text
main:
    li   t2, 2000000              # stores remaining
outer:
    li   t0, 0x10000000
    li   t1, 0x10001000      # 4 KiB region
inner:
    sw   t0, 0(t0)
    addi t0, t0, 4
    addi t2, t2, -1
    beq  t2, zero, done
    bne  t0, t1, inner
    j    outer
done:
    li   a7, 10
    ecall
