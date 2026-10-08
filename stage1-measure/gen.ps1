# Generate the Stage 1 measurement programs (RV32I assembly for Ripes).
#   fill_sw_<KiB>.s   : write N fresh bytes with sw (stride 4)
#   fill_sb_<KiB>.s   : same with sb (stride 1)
#   alu_<K>.s         : K iterations of an ALU-only loop
#   rewrite_<K>.s     : K sw stores cycling over the same 4 KiB
#   reread_<K>.s      : K lw loads cycling over the same 4 KiB
#   readfresh_<KiB>.s : lw over N bytes never written
#   empty.s           : exit only (fixed cost of exectime)
#   unroll_<K>.s, nottaken_<K>.s : branch cost on pipelined models
#   bigmap_*.s        : access speed against host map size
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$base = 0x10000000   # start of Ripes' default data segment

function Write-Asm($name, $body) {
    $body | Set-Content -Encoding ascii (Join-Path $dir $name)
}

function Fill($op, $stride, $n) {
    $end = '0x{0:X8}' -f ($base + $n)
    @"
    .text
main:
    li   t0, 0x10000000      # t0 = current guest address
    li   t1, $end      # t1 = one past the last byte to write
loop:
    $op   t0, 0(t0)           # store a non-zero value to a fresh address
    addi t0, t0, $stride
    bne  t0, t1, loop
    li   a7, 10              # exit
    ecall
"@
}

foreach ($kib in 64, 128, 1024, 2048, 4096, 8192, 16384) {
    $n = $kib * 1024
    Write-Asm "fill_sw_$kib.s" (Fill 'sw' 4 $n)
}
foreach ($kib in 64, 1024, 2048) {
    Write-Asm "fill_sb_$kib.s" (Fill 'sb' 1 ($kib * 1024))
}

foreach ($k in 20000, 40000, 2000000, 4000000) {
    Write-Asm "alu_$k.s" @"
    .text
main:
    li   t0, 0
    li   t1, $k
loop:
    addi t0, t0, 1
    bne  t0, t1, loop
    li   a7, 10
    ecall
"@
}

# Rewrite loop: 1024 words (4 KiB) visited over and over, K stores in total.
foreach ($k in 20000, 40000, 1000000, 2000000) {
    Write-Asm "rewrite_$k.s" @"
    .text
main:
    li   t2, $k              # stores remaining
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
"@
}


# Load loops. reread writes its 4 KiB once first; readfresh tests whether a
# read of unwritten memory allocates.
foreach ($k in 1000000, 2000000) {
    Write-Asm "reread_$k.s" @"
    .text
main:
    li   t0, 0x10000000      # write the 4 KiB region once
    li   t1, 0x10001000
init:
    sw   t0, 0(t0)
    addi t0, t0, 4
    bne  t0, t1, init
    li   t2, $k              # loads remaining
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
"@
}
foreach ($kib in 1024, 4096) {
    $end = '0x{0:X8}' -f ($base + $kib * 1024)
    Write-Asm "readfresh_$kib.s" @"
    .text
main:
    li   t0, 0x10000000
    li   t1, $end
loop:
    lw   t3, 0(t0)           # read memory that was never written
    addi t0, t0, 4
    bne  t0, t1, loop
    li   a7, 10
    ecall
"@
}

# Non-power-of-two sizes, to expose the bucket-array steps.
foreach ($kib in 1536, 3072, 6144, 12288) {
    Write-Asm "fill_sw_$kib.s" (Fill 'sw' 4 ($kib * 1024))
}

# Near-empty program: the fixed cost of exectime.
Write-Asm "empty.s" @"
    .text
main:
    li   a7, 10
    ecall
"@

# Branch cost on pipelined models.
#   unroll_<K>.s   : 4 addi + 1 taken bne per iteration
#   nottaken_<K>.s : ALU loop plus one never-taken beq per iteration
foreach ($k in 10000) {
    Write-Asm "unroll_$k.s" @"
    .text
main:
    li   t0, 0
    li   t1, $k
loop:
    addi t0, t0, 1
    addi t2, t2, 1
    addi t3, t3, 1
    addi t4, t4, 1
    bne  t0, t1, loop
    li   a7, 10
    ecall
"@
    Write-Asm "nottaken_$k.s" @"
    .text
main:
    li   t0, 0
    li   t1, $k
loop:
    addi t0, t0, 1
    beq  t0, zero, never     # never taken
    bne  t0, t1, loop
never:
    li   a7, 10
    ecall
"@
}

# Access speed against host map size: both programs touch the same 16 MiB,
# then do the same number of loads, over all 16 MiB or over 4 KiB.
foreach ($v in @(@{name='bigmap_readbig'; region=0x01000000; passes=4},
                 @{name='bigmap_readsmall'; region=0x00001000; passes=16384})) {
    $rend = '0x{0:X8}' -f ($base + $v.region)
    Write-Asm "$($v.name).s" @"
    .text
main:
    li   t0, 0x10000000      # phase 1: touch 16 MiB
    li   t1, 0x11000000
fill:
    sw   t0, 0(t0)
    addi t0, t0, 4
    bne  t0, t1, fill
    li   t2, $($v.passes)    # phase 2: read passes over the region
    li   t1, $rend
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
"@
}
