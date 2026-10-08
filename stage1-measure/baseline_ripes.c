/* Run the unmodified baseline solver.c on Ripes, to measure what Stage 1
 * otherwise has to estimate. Only the environment is supplied here:
 *   - stdout/stderr go to Ripes' PrintChar ecall (a7 = 11),
 *   - _exit uses Ripes' Exit2 ecall (a7 = 93, code in a0),
 *   - argv is fixed at build time, since Ripes passes no arguments.
 * malloc comes from picolibc, whose heap is the linker-script region between
 * .bss and the stack.
 */
#include <stdio.h>

#ifndef INPUT
#define INPUT "21345671111111"
#endif

static int ripes_putc(char c, FILE *file)
{
    (void) file;
    register int a0 __asm__("a0") = (unsigned char) c;
    register int a7 __asm__("a7") = 11;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
    return (unsigned char) c;
}

static FILE ripes_stdio =
    FDEV_SETUP_STREAM(ripes_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &ripes_stdio;
FILE *const stdout = &ripes_stdio;
FILE *const stderr = &ripes_stdio;

void _exit(int code)
{
    register int a0 __asm__("a0") = code;
    register int a7 __asm__("a7") = 93;
    __asm__ volatile("ecall" : : "r"(a0), "r"(a7));
    for (;;)
        ;
}

#define main solver_main
#include "solver.c"
#undef main

/* picolibc's crt0 does not call exit() when main returns; it spins on
 * `j .` forever (seen in the disassembly at _cstart+0x44). Ripes would then
 * never finish and never write its report, so leave through _exit here.
 */
int main(void)
{
    char *argv[] = {"solver", INPUT, NULL};
    _exit(solver_main(2, argv));
}
