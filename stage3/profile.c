/* Host profile of the search versions over all distance-11 states.
 *
 * All versions must search the same tree in the same order, so their node
 * counts and paths must match state by state.  v1's counters split the
 * generated children by outcome; these counts drive the Stage 3 argument.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"
#include "stats.h"

_Thread_local uint64_t ida_leaf, ida_cut_pd, ida_cut_opd;

int ida_solve_v0(uint16_t, uint16_t, uint8_t *);
int ida_solve_v1(uint16_t, uint16_t, uint8_t *);
int ida_solve_v2(uint16_t, uint16_t, uint8_t *);
int ida_solve_v3(uint16_t, uint16_t, uint8_t *);

static int (*const version[])(uint16_t, uint16_t, uint8_t *) = {
    ida_solve_v0, ida_solve_v1, ida_solve_v2, ida_solve_v3};
enum { NVER = sizeof version / sizeof version[0] };

static uint8_t dist[STATES];

static void exact_bfs(void)
{
    static uint32_t q[STATES];
    uint32_t head = 0, tail = 1;
    memset(dist, 0xFF, sizeof dist);
    dist[0] = 0;
    while (head < tail) {
        uint32_t r = q[head++];
        uint16_t p = (uint16_t) (r / NORI), o = (uint16_t) (r % NORI);
        for (int f = 0; f < 3; ++f) {
            uint16_t np = p, no = o;
            for (int t = 0; t < 3; ++t) {
                np = PT[f][np];
                no = OT[f][no];
                uint32_t n = (uint32_t) np * NORI + no;
                if (dist[n] == 0xFF) {
                    dist[n] = (uint8_t) (dist[r] + 1);
                    q[tail++] = n;
                }
            }
        }
    }
}

static void vector_of(const state_t *s, char out[15])
{
    for (int i = 0; i < CUBIES; ++i) {
        out[i] = (char) ('1' + s->p[i]);
        out[i + CUBIES] = (char) ('1' + s->o[i]);
    }
    out[14] = '\0';
}

typedef struct {
    uint64_t gen, exp, leaf, pd, opd;
} counts_t;

static counts_t run(int v, uint32_t r, uint8_t path[IDA_MAX_DEPTH], int *len)
{
    ida_generated = ida_expanded = ida_leaf = ida_cut_pd = ida_cut_opd = 0;
    *len = version[v]((uint16_t) (r / NORI), (uint16_t) (r % NORI), path);
    return (counts_t) {ida_generated, ida_expanded, ida_leaf, ida_cut_pd,
                       ida_cut_opd};
}

static void report(const char *name, uint32_t r)
{
    uint8_t path[IDA_MAX_DEPTH];
    int len;
    counts_t c = run(1, r, path, &len);
    uint64_t inner = c.gen - c.leaf;
    printf("%-22s d=%u  generated %7llu  expanded %6llu | leaf %5.1f%% |"
           " inner: PD cut %5.1f%%, OPD cut %5.1f%%, pass %5.1f%%\n",
           name, dist[r], (unsigned long long) c.gen,
           (unsigned long long) c.exp, 100.0 * c.leaf / c.gen,
           100.0 * c.pd / inner, 100.0 * c.opd / inner,
           100.0 * (inner - c.pd - c.opd) / inner);
}

int main(void)
{
    exact_bfs();

    /* Equivalence of the versions over every distance-11 state. */
    uint32_t n = 0, mismatch = 0, worst = 0;
    counts_t sum = {0}, max = {0};
    for (uint32_t r = 0; r < STATES; ++r) {
        if (dist[r] != 11)
            continue;
        uint8_t path0[IDA_MAX_DEPTH], path[IDA_MAX_DEPTH];
        int len0, len;
        counts_t c0 = run(0, r, path0, &len0);
        for (int v = 1; v < NVER; ++v) {
            counts_t c = run(v, r, path, &len);
            mismatch += len != len0 || c.gen != c0.gen || c.exp != c0.exp ||
                        memcmp(path, path0, (size_t) len0) != 0;
        }
        counts_t c = run(1, r, path, &len);
        sum.gen += c.gen;
        sum.exp += c.exp;
        sum.leaf += c.leaf;
        sum.pd += c.pd;
        sum.opd += c.opd;
        if (c.gen > max.gen) {
            max = c;
            worst = r;
        }
        ++n;
    }
    uint64_t inner = sum.gen - sum.leaf;
    printf("versions v0..v%d agree on all %u distance-11 states: %s\n",
           NVER - 1, n, mismatch ? "NO" : "yes");
    printf("d=11 totals: generated %llu, expanded %llu; leaf %.1f%% of"
           " generated; inner: PD cut %.1f%%, OPD cut %.1f%%, pass %.1f%%\n",
           (unsigned long long) sum.gen, (unsigned long long) sum.exp,
           100.0 * sum.leaf / sum.gen, 100.0 * sum.pd / inner,
           100.0 * sum.opd / inner,
           100.0 * (inner - sum.pd - sum.opd) / inner);

    /* Test vectors. */
    state_t s;
    char v[15];
    unrank(0, 0, &s);
    s = apply_move(s, 0);  /* R  */
    s = apply_move(s, 5);  /* B' */
    s = apply_move(s, 7);  /* D2 */
    vector_of(&s, v);
    printf("short scramble R B' D2 = %s\n", v);
    report("solved", 0);
    report(v, rank_p(&s) * NORI + rank_o(&s));
    {
        state_t t = {{1, 0, 2, 3, 4, 5, 6}, {0}};
        report("21345671111111", rank_p(&t) * NORI + rank_o(&t));
    }
    unrank(worst / NORI, worst % NORI, &s);
    vector_of(&s, v);
    report(v, worst);
    return mismatch != 0;
}
