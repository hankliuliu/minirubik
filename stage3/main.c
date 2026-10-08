/* Ripes harness: solves INPUT, checks the answer, prints it, and returns 0
 * (exit code) on success.  INPUT is the 14-character vector; EXPECT is its
 * distance, computed on the host.  No libc: output uses Ripes' PrintString
 * ecall (a7 = 4).
 */
#include <stdint.h>

#include "ida.h"
#include "tables.h"

#ifndef INPUT
#define INPUT "21345671111111"
#endif
#ifndef EXPECT
#define EXPECT 11
#endif

static const char input[] = INPUT;

static const uint8_t source[3][7] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][7] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};
static const uint8_t move_face[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
static const uint8_t move_turns[9] = {1, 2, 3, 1, 2, 3, 1, 2, 3};
static const char move_name[9][3] = {"R",  "R2", "R'", "B", "B2",
                                     "B'", "D",  "D2", "D'"};

static void print(const char *s)
{
    register const char *a0 __asm__("a0") = s;
    register int a7 __asm__("a7") = 4;
    __asm__ volatile("ecall" : : "r"(a0), "r"(a7) : "memory");
}

/* (a + b) mod 3 for a, b <= 2: the sum is at most 4. */
static uint32_t add_mod3(uint32_t a, uint32_t b)
{
#ifdef MOD3_BRANCHLESS
    int32_t t = (int32_t) (a + b) - 3;
    return (uint32_t) (t + ((t >> 31) & 3));
#else
    uint32_t s = a + b;
    if (s >= 3)
        s -= 3;
    return s;
#endif
}

static int fail(const char *why)
{
    print("FAIL: ");
    print(why);
    print("\n");
    return 1;
}

int main(void)
{
    uint8_t p[7], o[7], path[IDA_MAX_DEPTH], smaller[7];
    uint32_t seen = 0, sum = 0;

    for (int i = 0; i < 7; ++i) {
        uint32_t c = (uint32_t) (input[i] - '1');
        if (c > 6 || (seen >> c & 1))
            return fail("permutation digits");
        seen |= 1U << c;
        p[i] = (uint8_t) c;
        c = (uint32_t) (input[i + 7] - '1');
        if (c > 2)
            return fail("orientation digits");
        o[i] = (uint8_t) c;
        sum = add_mod3(sum, c);
    }
    if (input[14] != '\0' || sum != 0)
        return fail("length or twist sum");

    /* Lehmer rank with constant factors 6, 5, 4, 3, 2 as shift-add. */
    for (int i = 0; i < 7; ++i) {
        smaller[i] = 0;
        for (int j = i + 1; j < 7; ++j)
            smaller[i] += p[j] < p[i];
    }
    uint32_t pr = smaller[0];
    pr = (pr << 2) + (pr << 1) + smaller[1];
    pr = (pr << 2) + pr + smaller[2];
    pr = (pr << 2) + smaller[3];
    pr = (pr << 1) + pr + smaller[4];
    pr = (pr << 1) + smaller[5];
    uint32_t orank = 0;
    for (int i = 0; i < 6; ++i)
        orank = (orank << 1) + orank + o[i];

    int len = ida_solve((uint16_t) pr, (uint16_t) orank, path);
    if (len != EXPECT)
        return fail("length differs from the host distance");

    /* Print the moves and replay them on the 14-byte state (gate T5). */
    for (int k = 0; k < len; ++k) {
        uint32_t m = path[k];
        const uint8_t *src = source[move_face[m]], *tw = twist[move_face[m]];
        print(move_name[m]);
        print(k + 1 < len ? " " : "");
        for (uint32_t t = move_turns[m]; t; --t) {
            uint8_t np[7], no[7];
            for (int i = 0; i < 7; ++i) {
                np[i] = p[src[i]];
                no[i] = (uint8_t) add_mod3(o[src[i]], tw[i]);
            }
            for (int i = 0; i < 7; ++i) {
                p[i] = np[i];
                o[i] = no[i];
            }
        }
    }
    print("\n");
    for (int i = 0; i < 7; ++i)
        if (p[i] != i || o[i] != 0)
            return fail("path does not reach solved");
    print("OK\n");
    return 0;
}
