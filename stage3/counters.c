/* Search counters for IDA_STATS builds of the stage3 versions. */
#include "stats.h"

_Thread_local uint64_t ida_generated, ida_expanded;
_Thread_local uint64_t ida_leaf, ida_cut_pd, ida_cut_opd;
