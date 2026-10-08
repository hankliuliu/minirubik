/* Lookup tables used by ida.c and check.c, defined in tables.c, which
 * gen_tables.c generates.
 */
#ifndef TABLES_H
#define TABLES_H

#include <stdint.h>

enum {
    NPERM = 5040,  /* 7! permutations of the 7 free corners */
    NORI = 729,    /* 3^6 orientations; the 7th twist is implied */
    NPOS = 42,     /* positions of cubies 0 and 3: 7 * 6 */
    OSTRIDE = 64,  /* row stride of OPD: a power of two, so no multiply */
};

/* Quarter turn of face f (R, B, D) on each coordinate. */
extern const uint16_t PT[3][NPERM];
extern const uint16_t OT[3][NORI];

/* p -> positions of cubies 0 and 3 (0..41). */
extern const uint8_t POS[NPERM];

/* Pattern databases: exact distances in two abstractions. */
extern const uint8_t PD[NPERM];             /* permutation */
extern const uint8_t OPD[NORI * OSTRIDE];   /* orientation x POS */

#define OPD_INDEX(o, p) (((uint32_t) (o) << 6) | POS[p])

#endif
