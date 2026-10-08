/* v2: v1 plus a separate last level.
 *
 * At rem == 0 a child passes only if it is solved, so the PD and OPD
 * lookups become a test of np == 0.  The orientation is advanced only
 * when np == 0, which is rare.
 */
#include "ida.h"
#include "stats.h"
#include "tables.h"

#ifdef IDA_STATS
static inline int fits(uint32_t np, uint32_t no, uint32_t rem)
{
    COUNT(ida_generated);
    if (PD[np] > rem) {
        COUNT(ida_cut_pd);
        return 0;
    }
    if (OPD[OPD_INDEX(no, np)] > rem) {
        COUNT(ida_cut_opd);
        return 0;
    }
    return 1;
}
#define LEAF() (COUNT(ida_generated), COUNT(ida_leaf))
#else
#define fits(np, no, rem) \
    (PD[np] <= (rem) && OPD[OPD_INDEX(no, np)] <= (rem))
#define LEAF() ((void) 0)
#endif

int ida_solve(uint16_t p0, uint16_t o0, uint8_t path[IDA_MAX_DEPTH])
{
    uint16_t fp[IDA_MAX_DEPTH], fo[IDA_MAX_DEPTH];  /* node at depth g */
    uint8_t ff[IDA_MAX_DEPTH], ft[IDA_MAX_DEPTH];   /* move taken from it */
    uint32_t hp = PD[p0], ho = OPD[OPD_INDEX(o0, p0)];

    if (hp == 0 && ho == 0)
        return 0;
    for (uint32_t bound = hp > ho ? hp : ho; bound <= IDA_MAX_DEPTH;
         ++bound) {
        uint32_t g = 0, rem = bound - 1, f = 0, last = 3, t;
        uint32_t p = p0, o = o0, np, no;
        const uint16_t *pt, *ot;
        COUNT(ida_expanded);
        if (rem == 0)
            goto leaf;
    face:
        if (f == last)
            ++f;
        if (f >= 3) /* also when f == last == 3 at the root */
            goto up;
        pt = PT[f];
        ot = OT[f];
        np = pt[p];
        no = ot[o];
        t = 1;
        if (fits(np, no, rem))
            goto down;
    turn2:
        np = pt[np];
        no = ot[no];
        t = 2;
        if (fits(np, no, rem))
            goto down;
    turn3:
        np = pt[np];
        no = ot[no];
        t = 3;
        if (fits(np, no, rem))
            goto down;
    next:
        ++f;
        goto face;
    down:
        fp[g] = (uint16_t) p;
        fo[g] = (uint16_t) o;
        ff[g] = (uint8_t) f;
        ft[g] = (uint8_t) t;
        ++g;
        --rem;
        last = f;
        f = 0;
        p = np;
        o = no;
        COUNT(ida_expanded);
        if (rem != 0)
            goto face;
    leaf:
        /* Children are at depth bound: only the solved state passes. */
        for (f = 0; f < 3; ++f) {
            if (f == last)
                continue;
            pt = PT[f];
            ot = OT[f];
            np = pt[p];
            LEAF();
            if (np == 0 && ot[o] == 0) {
                t = 1;
                goto solved;
            }
            np = pt[np];
            LEAF();
            if (np == 0 && ot[ot[o]] == 0) {
                t = 2;
                goto solved;
            }
            np = pt[np];
            LEAF();
            if (np == 0 && ot[ot[ot[o]]] == 0) {
                t = 3;
                goto solved;
            }
        }
    up:
        if (g == 0)
            continue;
        np = p; /* the node being left is the child of depth g - 1 */
        no = o;
        --g;
        ++rem;
        p = fp[g];
        o = fo[g];
        f = ff[g];
        t = ft[g];
        last = g ? ff[g - 1] : 3;
        pt = PT[f];
        ot = OT[f];
        if (t == 1)
            goto turn2;
        if (t == 2)
            goto turn3;
        goto next;
    solved:
        path[g] = (uint8_t) (f * 3 + t - 1);
        for (uint32_t i = 0; i < g; ++i)
            path[i] = (uint8_t) (ff[i] * 3U + ft[i] - 1U);
        return (int) bound;
    }
    return -1;
}
