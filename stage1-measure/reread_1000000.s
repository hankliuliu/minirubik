    .text
main:
    li   t0, 0x10000000      # write the 4 KiB region once
    li   t1, 0x10001000
init:
    sw   t0, 0(t0)
    addi t0, t0, 4
    bne  t0, t1, init
    li   t2, 1000000              # loads remaining
outer:
    li   t0, 0x10000000
inner:
    lw   t3, 0(t0)
    addi t0, t0, 4
    addi t2, t2, -1
    beq  t2, zero, done
    bne  t0, t1, inner
    j    outer
done:
    li   a7, 10
    ecall
