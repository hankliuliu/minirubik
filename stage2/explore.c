/* Stage 2 design exploration (host only).
 *
 * Runs IDA* with each candidate heuristic over every distance-11 state and
 * reports nodes generated and expanded.  A heuristic is the max of the
 * pattern databases named on the command line:
 *
 *   P        permutation only            (5040 entries)
 *   O        orientation only            (729 entries)
 *   OP:abc   orientation x positions of cubies a,b,c
 *   PT:ab    permutation x twists of cubies a,b
 *
 * Example: ./explore P,O P,OP:01 P,OP:012
 *
 * Each heuristic is expanded into a full-rank table H[p * 729 + o] for
 * speed; node counts are the same as with separate lookups.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { CUBIES = 7, NP = 5040, NO = 729, STATES = NP * NO };

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

static state_t quarter_turn(state_t s, int f)
{
    state_t r;
    for (int i = 0; i < CUBIES; ++i) {
        r.p[i] = s.p[source[f][i]];
        r.o[i] = (uint8_t) ((s.o[source[f][i]] + twist[f][i]) % 3);
    }
    return r;
}

static uint32_t rank_p(const state_t *s)
{
    uint32_t p = 0;
    for (int i = 0; i < CUBIES; ++i) {
        int smaller = 0;
        for (int j = i + 1; j < CUBIES; ++j)
            smaller += s->p[j] < s->p[i];
        p = p * (CUBIES - i) + smaller;
    }
    return p;
}

static uint32_t rank_o(const state_t *s)
{
    uint32_t o = 0;
    for (int i = 0; i < 6; ++i)
        o = o * 3 + s->o[i];
    return o;
}

static void unrank(uint32_t p, uint32_t o, state_t *s)
{
    uint8_t avail[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t f = 720;
    int sum = 0;
    for (int i = 0; i < CUBIES; ++i) {
        int q = p / f;
        p %= f;
        s->p[i] = avail[q];
        for (int j = q; j + 1 < CUBIES - i; ++j)
            avail[j] = avail[j + 1];
        if (i < 6)
            f /= 6 - i;
    }
    for (int i = 6; i-- > 0;) {
        s->o[i] = o % 3;
        sum += s->o[i];
        o /= 3;
    }
    s->o[6] = (3 - sum % 3) % 3;
}

static uint16_t PT[3][NP], OT[3][NO];
static uint8_t *dist;

static void build_transitions(void)
{
    state_t s;
    for (uint32_t r = 0; r < NP; ++r) {
        unrank(r, 0, &s);
        for (int f = 0; f < 3; ++f) {
            state_t n = quarter_turn(s, f);
            PT[f][r] = rank_p(&n);
        }
    }
    for (uint32_t r = 0; r < NO; ++r) {
        unrank(0, r, &s);
        for (int f = 0; f < 3; ++f) {
            state_t n = quarter_turn(s, f);
            OT[f][r] = rank_o(&n);
        }
    }
}

static void build_exact(void)
{
    uint32_t *q = malloc(sizeof *q * STATES), head = 0, tail = 1;
    dist = malloc(STATES);
    memset(dist, 0xFF, STATES);
    dist[0] = 0;
    q[0] = 0;
    while (head < tail) {
        uint32_t r = q[head++];
        uint16_t p = r / NO, o = r % NO;
        for (int f = 0; f < 3; ++f) {
            uint16_t np = p, no = o;
            for (int t = 0; t < 3; ++t) {
                np = PT[f][np];
                no = OT[f][no];
                uint32_t n = (uint32_t) np * NO + no;
                if (dist[n] == 0xFF) {
                    dist[n] = dist[r] + 1;
                    q[tail++] = n;
                }
            }
        }
    }
    free(q);
    if (tail != STATES) {
        fprintf(stderr, "exact BFS incomplete\n");
        exit(1);
    }
}

/* A projection: kind 'P', 'O', 'Q' (OP), 'T' (PT) and a cubie subset. */
typedef struct {
    char kind;
    int k, cubie[CUBIES];
    uint32_t size;
} proj_t;

static uint32_t project(const proj_t *pr, const state_t *s)
{
    uint8_t where[CUBIES];
    for (int i = 0; i < CUBIES; ++i)
        where[s->p[i]] = i;
    uint32_t x = 0;
    switch (pr->kind) {
    case 'P':
        return rank_p(s);
    case 'O':
        return rank_o(s);
    case 'Q': /* orientation x base-7 positions of the chosen cubies */
        for (int j = 0; j < pr->k; ++j)
            x = x * 7 + where[pr->cubie[j]];
        return rank_o(s) * pr->size / NO + x;
    default: /* 'T': permutation x base-3 twists of the chosen cubies */
        for (int j = 0; j < pr->k; ++j)
            x = x * 3 + s->o[where[pr->cubie[j]]];
        return rank_p(s) * (pr->size / NP) + x;
    }
}

/* Breadth-first search over the abstract space through representatives. */
static uint8_t *build_pdb(const proj_t *pr, int *maxd, uint32_t *filled)
{
    uint8_t *d = malloc(pr->size);
    state_t *q = malloc(sizeof *q * pr->size);
    uint32_t head = 0, tail = 1;
    memset(d, 0xFF, pr->size);
    unrank(0, 0, &q[0]);
    d[project(pr, &q[0])] = 0;
    *maxd = 0;
    while (head < tail) {
        state_t s = q[head++];
        uint8_t ds = d[project(pr, &s)];
        for (int f = 0; f < 3; ++f) {
            state_t n = s;
            for (int t = 0; t < 3; ++t) {
                n = quarter_turn(n, f);
                uint32_t x = project(pr, &n);
                if (d[x] == 0xFF) {
                    d[x] = ds + 1;
                    if (ds + 1 > *maxd)
                        *maxd = ds + 1;
                    q[tail++] = n;
                }
            }
        }
    }
    free(q);
    *filled = tail;
    return d;
}

static int parse_proj(const char *t, proj_t *pr)
{
    memset(pr, 0, sizeof *pr);
    if (!strcmp(t, "P")) {
        pr->kind = 'P';
        pr->size = NP;
    } else if (!strcmp(t, "O")) {
        pr->kind = 'O';
        pr->size = NO;
    } else if (!strncmp(t, "OP:", 3) || !strncmp(t, "PT:", 3)) {
        pr->kind = t[0] == 'O' ? 'Q' : 'T';
        for (const char *c = t + 3; *c; ++c)
            pr->cubie[pr->k++] = *c - '0';
        uint32_t m = 1;
        for (int j = 0; j < pr->k; ++j)
            m *= pr->kind == 'Q' ? 7 : 3;
        pr->size = (pr->kind == 'Q' ? NO : NP) * m;
    } else
        return 0;
    return 1;
}

/* ---- IDA* over (p, o) with the full-rank heuristic H ---- */

static const uint8_t *H;
static uint64_t expanded, generated;
static int bound;

static int dfs(uint16_t p, uint16_t o, int g, int last)
{
    if (p == 0 && o == 0)
        return 1;
    ++expanded;
    for (int f = 0; f < 3; ++f) {
        if (f == last)
            continue;
        uint16_t np = p, no = o;
        for (int t = 0; t < 3; ++t) {
            np = PT[f][np];
            no = OT[f][no];
            ++generated;
            if (g + 1 + H[(uint32_t) np * NO + no] <= bound &&
                dfs(np, no, g + 1, f))
                return 1;
        }
    }
    return 0;
}

static int solve(uint32_t r)
{
    uint16_t p = r / NO, o = r % NO;
    for (bound = H[r];; ++bound)
        if (dfs(p, o, 0, -1))
            return bound;
}

int main(int argc, char **argv)
{
    build_transitions();
    build_exact();
    uint32_t hard[2644], nhard = 0, target = 0;
    for (uint32_t r = 0; r < STATES; ++r)
        if (dist[r] == 11)
            hard[nhard++] = r;
    {
        /* 21345671111111 */
        state_t s = {{1, 0, 2, 3, 4, 5, 6}, {0, 0, 0, 0, 0, 0, 0}};
        target = rank_p(&s) * NO + rank_o(&s);
    }
    printf("distance-11 states: %u; 21345671111111 has distance %u\n\n",
           nhard, dist[target]);

    uint8_t *h = malloc(STATES);
    for (int a = 1; a < argc; ++a) {
        proj_t pr[4];
        uint8_t *pdb[4];
        int n = 0;
        char spec[128];
        snprintf(spec, sizeof spec, "%s", argv[a]);
        printf("== heuristic max(%s)\n", argv[a]);
        for (char *t = strtok(spec, ","); t; t = strtok(NULL, ",")) {
            int maxd;
            uint32_t filled;
            if (!parse_proj(t, &pr[n])) {
                fprintf(stderr, "bad projection %s\n", t);
                return 1;
            }
            pdb[n] = build_pdb(&pr[n], &maxd, &filled);
            printf("   %-8s %6u reachable entries, max %d\n", t, filled, maxd);
            ++n;
        }
        double sum = 0, dsum = 0;
        uint32_t bad = 0;
        for (uint32_t r = 0; r < STATES; ++r) {
            state_t s;
            unrank(r / NO, r % NO, &s);
            uint8_t v = 0;
            for (int j = 0; j < n; ++j) {
                uint8_t x = pdb[j][project(&pr[j], &s)];
                if (x > v)
                    v = x;
            }
            h[r] = v;
            sum += v;
            dsum += dist[r];
            bad += v > dist[r];
        }
        H = h;
        printf("   mean h over all states %.3f (mean d %.3f), inadmissible %u\n",
               sum / STATES, dsum / STATES, bad);

        clock_t c0 = clock();
        uint64_t emax = 0, gmax = 0, esum = 0, gsum = 0;
        uint32_t wrong = 0, worst = 0;
        for (uint32_t i = 0; i < nhard; ++i) {
            expanded = generated = 0;
            wrong += solve(hard[i]) != 11;
            esum += expanded;
            gsum += generated;
            if (generated > gmax) {
                gmax = generated;
                emax = expanded;
                worst = hard[i];
            }
        }
        double sec = (double) (clock() - c0) / CLOCKS_PER_SEC;
        expanded = generated = 0;
        solve(target);
        printf("   d=11: generated mean %.0f max %llu (expanded %llu, rank %u)"
               " | 2134567: generated %llu expanded %llu | wrong %u | %.1f s\n\n",
               (double) gsum / nhard, (unsigned long long) gmax,
               (unsigned long long) emax, worst,
               (unsigned long long) generated, (unsigned long long) expanded,
               wrong, sec);
        for (int j = 0; j < n; ++j)
            free(pdb[j]);
    }
    return 0;
}
