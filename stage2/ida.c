/* Iterative-deepening A* over the coordinate pair (p, o).
 *
 * No recursion, heap, multiply or divide: the state is the index pair
 * (p, o), and the depth-first stack is arrays of IDA_MAX_DEPTH entries.
 *
 * h(p, o) = max(PD[p], OPD[o, POS[p]]) is admissible (gate H1) and is 0
 * only at the solved state (gate H2).  The max is not computed:
 * g + 1 + max(a, b) <= bound  <=>  a <= rem and b <= rem,
 * where rem = bound - g - 1.
 */
#include "ida.h"
#include "tables.h"

#ifdef IDA_STATS
_Thread_local uint64_t ida_generated, ida_expanded;
#define COUNT(x) (++(x))
#else
#define COUNT(x) ((void) 0)
#endif

int ida_solve(uint16_t p0, uint16_t o0, uint8_t path[IDA_MAX_DEPTH])
{
    /* p[g], o[g]: the node at depth g.  face[g], turn[g]: the move from
     * depth g currently being tried; p[g + 1], o[g + 1] hold its result.
     */
    uint16_t p[IDA_MAX_DEPTH + 1], o[IDA_MAX_DEPTH + 1];
    uint8_t face[IDA_MAX_DEPTH], turn[IDA_MAX_DEPTH];
    uint32_t hp = PD[p0], ho = OPD[OPD_INDEX(o0, p0)];

    if (hp == 0 && ho == 0)
        return 0;
    for (uint32_t bound = hp > ho ? hp : ho; bound <= IDA_MAX_DEPTH;
         ++bound) {
        uint32_t g = 0;
        p[0] = p[1] = p0;
        o[0] = o[1] = o0;
        face[0] = 0;
        turn[0] = 0;
        COUNT(ida_expanded);
        for (;;) {
            if (turn[g] == 3) {
                /* Face exhausted: next face, skipping the one just turned. */
                uint32_t f = face[g] + 1U;
                if (g > 0 && f == face[g - 1])
                    ++f;
                if (f == 3) {
                    if (g == 0)
                        break; /* bound exhausted */
                    --g;       /* backtrack; p[g + 1] still holds the child */
                    continue;
                }
                face[g] = (uint8_t) f;
                turn[g] = 0;
                p[g + 1] = p[g];
                o[g + 1] = o[g];
            }
            uint16_t np = PT[face[g]][p[g + 1]];
            uint16_t no = OT[face[g]][o[g + 1]];
            p[g + 1] = np;
            o[g + 1] = no;
            ++turn[g];
            COUNT(ida_generated);

            uint32_t rem = bound - 1 - g;
            if (PD[np] > rem || OPD[OPD_INDEX(no, np)] > rem)
                continue;
            if (rem == 0) {
                /* Both distances are 0: solved.  Testing only at
                 * depth == bound suffices, since the distance is at least
                 * bound (bound starts at h(root); smaller bounds failed).
                 */
                for (uint32_t i = 0; i < bound; ++i)
                    path[i] = (uint8_t) (face[i] * 3U + turn[i] - 1U);
                return (int) bound;
            }
            ++g;
            COUNT(ida_expanded);
            face[g] = face[g - 1] == 0 ? 1 : 0;
            turn[g] = 0;
            p[g + 1] = np;
            o[g + 1] = no;
        }
    }
    return -1;
}
