#ifndef IDA_H
#define IDA_H

#include <stdint.h>

enum { IDA_MAX_DEPTH = 11 };

/* Writes a shortest solution of state (p, o) to path as move indices
 * (face * 3 + quarter turns - 1) and returns its length, or -1 if none is
 * found within IDA_MAX_DEPTH moves.
 */
int ida_solve(uint16_t p, uint16_t o, uint8_t path[IDA_MAX_DEPTH]);

#ifdef IDA_STATS
extern _Thread_local uint64_t ida_generated, ida_expanded;
#endif

#endif
