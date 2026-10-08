/* Reference cube model from solver.c (host only; uses division).
 * gen_tables.c builds every table from it; check.c verifies them against it.
 */
#ifndef CUBE_H
#define CUBE_H

#include <stdint.h>

#include "tables.h"

enum { CUBIES = 7, STATES = NPERM * NORI, MOVES = 9 };

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

static inline state_t quarter_turn(state_t s, int face)
{
    state_t r;
    for (int i = 0; i < CUBIES; ++i) {
        r.p[i] = s.p[source[face][i]];
        r.o[i] = (uint8_t) ((s.o[source[face][i]] + twist[face][i]) % 3);
    }
    return r;
}

/* move = face * 3 + (quarter turns - 1), the order of R R2 R' B ... D'. */
static inline state_t apply_move(state_t s, int move)
{
    for (int t = 0; t <= move % 3; ++t)
        s = quarter_turn(s, move / 3);
    return s;
}

static inline uint32_t rank_p(const state_t *s)
{
    uint32_t p = 0;
    for (int i = 0; i < CUBIES; ++i) {
        uint32_t smaller = 0;
        for (int j = i + 1; j < CUBIES; ++j)
            smaller += s->p[j] < s->p[i];
        p = p * (uint32_t) (CUBIES - i) + smaller;
    }
    return p;
}

static inline uint32_t rank_o(const state_t *s)
{
    uint32_t o = 0;
    for (int i = 0; i < 6; ++i)
        o = o * 3 + s->o[i];
    return o;
}

static inline void unrank(uint32_t p, uint32_t o, state_t *s)
{
    uint8_t avail[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t f = 720, sum = 0;
    for (int i = 0; i < CUBIES; ++i) {
        uint32_t q = p / f;
        p %= f;
        s->p[i] = avail[q];
        for (uint32_t j = q; j + 1 < (uint32_t) (CUBIES - i); ++j)
            avail[j] = avail[j + 1];
        if (i < 6)
            f /= (uint32_t) (6 - i);
    }
    for (int i = 6; i-- > 0;) {
        s->o[i] = (uint8_t) (o % 3);
        sum += s->o[i];
        o /= 3;
    }
    s->o[6] = (uint8_t) ((3 - sum % 3) % 3);
}

/* a, b = positions of cubies 0 and 3; code a * 6 + (b > a ? b - 1 : b),
 * in 0..41.
 */
static inline uint32_t pos_code(const state_t *s)
{
    uint32_t a = 0, b = 0;
    for (uint32_t i = 0; i < CUBIES; ++i) {
        if (s->p[i] == 0)
            a = i;
        if (s->p[i] == 3)
            b = i;
    }
    return a * 6 + (b > a ? b - 1 : b);
}

#endif
