"""Predict the retired-instruction count of the baseline BFS loop on RV32I.

Counts are read off baseline_21345671111111.dis (gcc 13.2 -O2 -march=rv32i).
The two libgcc calls per dequeued state are simulated instruction by
instruction, because their cost depends on the dividend.
"""
STATES = 5040 * 729
DIVISOR = 729


def udivsi3(a, b=DIVISOR):
    """Dynamic instruction count of __hidden___udivsi3(a, b) in the .dis."""
    n = 4                      # mv, mv, li, beqz (b != 0)
    n += 2                     # li a3,1 ; bgeu a2,a1
    d, q = b, 1
    if not (d >= a):           # loop 1: double the divisor until it passes a
        while True:
            n += 4             # blez, slli, slli, bltu
            d <<= 1
            q <<= 1
            if not (d < a):
                break
    n += 1                     # li a0,0
    r = a
    while True:                # loop 2: one restoring step per quotient bit
        n += 1                 # bltu a1,a2
        if not (r < d):
            n += 2             # sub, or
            r -= d
        n += 3                 # srli, srli, bnez
        q >>= 1
        d >>= 1
        if q == 0:
            break
    return n + 1               # ret


def umodsi3(a):
    return 4 + udivsi3(a)      # mv t0,ra ; jal ; ... ; mv a0,a1 ; jr t0


div = sum(udivsi3(h) + umodsi3(h) for h in range(STATES))
per_state = 15 + 3 * (12 + 4) + 3      # dequeue+calls, 3 faces, loop end
edges = STATES * 9
bfs = STATES * per_state + edges * 23 + (STATES - 1) * 9 + div

print(f"division (udiv+umod) total : {div:>15,}  ({div / STATES:.1f} per state)")
print(f"per-state fixed            : {STATES * per_state:>15,}")
print(f"per-edge (23 each)         : {edges * 23:>15,}")
print(f"new-state extra (9 each)   : {(STATES - 1) * 9:>15,}")
print(f"BFS loop total (predicted) : {bfs:>15,}")
