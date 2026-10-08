/* Host-side correctness gates H1-H4 for tables.c and ida.c, against an exact
 * BFS over all 3,674,160 states.  Tables are read as linked and compared
 * with the reference model in cube.h.
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"
#include "ida.h"
#include "tables.h"

static int fails;
#define CHECK(cond, ...)                          \
    do {                                          \
        if (!(cond) && fails++ < 20) {            \
            printf("    FAIL %s:%d: ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                  \
            putchar('\n');                        \
        }                                         \
    } while (0)

static uint8_t *dist;

static void vector_of(uint32_t r, char out[15])
{
    state_t s;
    unrank(r / NORI, r % NORI, &s);
    for (int i = 0; i < CUBIES; ++i) {
        out[i] = (char) ('1' + s.p[i]);
        out[i + CUBIES] = (char) ('1' + s.o[i]);
    }
    out[14] = '\0';
}

/* H2a: PT, OT and POS, entry by entry against cube.h. */
static void h2_transitions(void)
{
    static uint8_t seen[NPERM];
    static uint32_t fiber[NPOS];
    state_t s;
    for (int f = 0; f < 3; ++f) {
        memset(seen, 0, sizeof seen);
        for (uint32_t r = 0; r < NPERM; ++r) {
            unrank(r, 0, &s);
            state_t n = quarter_turn(s, f);
            CHECK(PT[f][r] == rank_p(&n), "PT[%d][%u]", f, r);
            CHECK(PT[f][r] < NPERM && !seen[PT[f][r]]++, "PT[%d] not a bijection at %u", f, r);
            uint32_t x = r;
            for (int t = 0; t < 4; ++t)
                x = PT[f][x];
            CHECK(x == r, "PT[%d]^4 != identity at %u", f, r);
        }
        memset(seen, 0, sizeof seen);
        for (uint32_t r = 0; r < NORI; ++r) {
            unrank(0, r, &s);
            state_t n = quarter_turn(s, f);
            CHECK(OT[f][r] == rank_o(&n), "OT[%d][%u]", f, r);
            CHECK(OT[f][r] < NORI && !seen[OT[f][r]]++, "OT[%d] not a bijection at %u", f, r);
            uint32_t x = r;
            for (int t = 0; t < 4; ++t)
                x = OT[f][x];
            CHECK(x == r, "OT[%d]^4 != identity at %u", f, r);
        }
    }
    for (uint32_t r = 0; r < NPERM; ++r) {
        unrank(r, 0, &s);
        CHECK(POS[r] == pos_code(&s), "POS[%u]", r);
        CHECK(POS[r] < NPOS, "POS[%u] = %u out of range", r, POS[r]);
        if (POS[r] < NPOS)
            ++fiber[POS[r]];
    }
    for (uint32_t i = 0; i < NPOS; ++i)
        CHECK(fiber[i] == NPERM / NPOS, "POS fiber %u has %u permutations", i, fiber[i]);
    printf("H2a PT, OT, POS match cube.h; each face is a bijection of order 4;"
           " POS fibers all %d\n", NPERM / NPOS);
}

/* Exact distances by BFS over PT and OT (verified by H2a). */
static void exact_bfs(void)
{
    uint32_t *q = malloc(sizeof *q * STATES), head = 0, tail = 1;
    uint32_t count[16] = {0};
    dist = malloc(STATES);
    if (!q || !dist) {
        perror("malloc");
        exit(1);
    }
    memset(dist, 0xFF, STATES);
    dist[0] = 0;
    q[0] = 0;
    while (head < tail) {
        uint32_t r = q[head++];
        uint16_t p = (uint16_t) (r / NORI), o = (uint16_t) (r % NORI);
        ++count[dist[r]];
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
    free(q);
    CHECK(tail == STATES, "BFS reached %u states", tail);
    CHECK(count[11] == 2644 && count[12] == 0, "diameter is not 11");
    printf("BFS %u states, by distance:", tail);
    for (int d = 0; d < 12; ++d)
        printf(" %u", count[d]);
    putchar('\n');
}

/* H2b: each PDB entry equals the minimum exact distance over its fiber.
 * An abstract solution S acts bijectively on concrete states, so the fiber
 * contains a state that S solves; hence equality.
 */
static void h2_pdbs(void)
{
    static uint8_t min_p[NPERM], min_op[NORI * OSTRIDE];
    uint32_t max_pd = 0, max_opd = 0;
    memset(min_p, 0xFF, sizeof min_p);
    memset(min_op, 0xFF, sizeof min_op);
    for (uint32_t r = 0; r < STATES; ++r) {
        uint32_t p = r / NORI, o = r % NORI, i = OPD_INDEX(o, p);
        if (dist[r] < min_p[p])
            min_p[p] = dist[r];
        if (dist[r] < min_op[i])
            min_op[i] = dist[r];
    }
    for (uint32_t p = 0; p < NPERM; ++p) {
        CHECK(PD[p] != 0xFF, "PD[%u] unfilled", p);
        CHECK(PD[p] == min_p[p], "PD[%u] = %u, fiber minimum %u", p, PD[p], min_p[p]);
        CHECK((PD[p] == 0) == (p == 0), "PD[%u] = 0 off the solved state", p);
        if (PD[p] > max_pd)
            max_pd = PD[p];
    }
    uint32_t solved = OPD_INDEX(0, 0);
    for (uint32_t i = 0; i < NORI * OSTRIDE; ++i) {
        if ((i & (OSTRIDE - 1)) >= NPOS) {
            CHECK(OPD[i] == 0xFF, "OPD padding %u = %u", i, OPD[i]);
            continue;
        }
        CHECK(OPD[i] != 0xFF, "OPD[%u] unfilled", i);
        CHECK(OPD[i] == min_op[i], "OPD[%u] = %u, fiber minimum %u", i, OPD[i], min_op[i]);
        CHECK((OPD[i] == 0) == (i == solved), "OPD[%u] = 0 off the solved state", i);
        if (OPD[i] > max_opd)
            max_opd = OPD[i];
    }
    CHECK(max_pd == 7 && max_opd == 8, "maxima %u, %u", max_pd, max_opd);
    printf("H2b PD: 5040 entries, max %u, PD[0] = %u; OPD: %d entries, max %u,"
           " OPD[%u] = %u; every entry = its fiber minimum; padding 0xFF\n",
           max_pd, PD[0], NORI * NPOS, max_opd, solved, OPD[solved]);
}

static void h1(void)
{
    uint32_t over = 0, exact = 0, gap[12] = {0};
    double sum = 0;
    for (uint32_t r = 0; r < STATES; ++r) {
        uint32_t p = r / NORI, o = r % NORI;
        uint32_t a = PD[p], b = OPD[OPD_INDEX(o, p)], h = a > b ? a : b;
        CHECK(h <= dist[r], "h = %u > d = %u at rank %u", h, dist[r], r);
        over += h > dist[r];
        exact += h == dist[r];
        if (h <= dist[r])
            ++gap[dist[r] - h];
        sum += h;
    }
    printf("H1  h <= d at all %d states (%u violations); h == d at %u;"
           " mean h %.3f; d - h:", STATES, over, exact, sum / STATES);
    for (int g = 0; g < 12 && gap[g]; ++g)
        printf(" %u", gap[g]);
    putchar('\n');
}

static void h3(void)
{
    uint32_t *gen = malloc(sizeof *gen * STATES);
    uint32_t wrong = 0, unsolved = 0;
    if (!gen) {
        perror("malloc");
        exit(1);
    }
    double t0 = omp_get_wtime();
#pragma omp parallel for schedule(dynamic, 4096) reduction(+ : wrong, unsolved)
    for (uint32_t r = 0; r < STATES; ++r) {
        uint8_t path[IDA_MAX_DEPTH];
        state_t s;
        ida_generated = ida_expanded = 0;
        int len = ida_solve((uint16_t) (r / NORI), (uint16_t) (r % NORI), path);
        gen[r] = (uint32_t) ida_generated;
        wrong += len != dist[r];
        /* Replay on the reference model, independent of the tables. */
        unrank(r / NORI, r % NORI, &s);
        for (int i = 0; i < len; ++i)
            s = apply_move(s, path[i]);
        unsolved += len < 0 || rank_p(&s) != 0 || rank_o(&s) != 0;
    }
    double sec = omp_get_wtime() - t0;
    CHECK(wrong == 0, "%u states with length != distance", wrong);
    CHECK(unsolved == 0, "%u paths do not reach solved", unsolved);
    printf("H3  all %d states: length == exact distance (%u wrong), path"
           " solves on cube.h (%u fail); %.1f s on %d threads\n",
           STATES, wrong, unsolved, sec, omp_get_max_threads());

    /* Search cost over the distance-11 states. */
    uint32_t worst = 0, n = 0;
    double sum = 0;
    for (uint32_t r = 0; r < STATES; ++r)
        if (dist[r] == 11) {
            ++n;
            sum += gen[r];
            if (gen[r] > gen[worst] || dist[worst] != 11)
                worst = r;
        }
    char v[15];
    vector_of(worst, v);
    printf("    d=11: %u states, generated mean %.0f, max %u at rank %u"
           " (vector %s)\n",
           n, sum / n, gen[worst], worst, v);
    free(gen);
}

int main(void)
{
    h2_transitions();
    exact_bfs();
    h2_pdbs();
    h1();
    h3();
    printf("H4  not applicable: no table is packed below one byte per entry\n");
    printf(fails ? "FAILED (%d)\n" : "all gates pass\n", fails);
    return fails != 0;
}
