/* Extra search counters for the host profile (IDA_STATS builds only). */
#ifndef STATS_H
#define STATS_H

#include <stdint.h>

#include "ida.h"

extern _Thread_local uint64_t ida_leaf, ida_cut_pd, ida_cut_opd;

#ifdef IDA_STATS
#define COUNT(x) (++(x))
#else
#define COUNT(x) ((void) 0)
#endif

#endif
