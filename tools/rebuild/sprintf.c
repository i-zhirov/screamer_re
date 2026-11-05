/* The rebuilt `sprintf` module.
 *
 * The original sits at linear 0x4d32c in STARTH.EXE and holds 18 instructions
 * and 48 bytes: the register-save prologue, a 4-byte local, the `va_start`
 * address of the first variable argument, the format and the buffer arguments
 * from the stack, the fourth argument `0x3d319`, the format-engine call at
 * `0xd68c0`, the terminator write, and the epilogue.  The function holds no
 * EBP frame, so the rebuild uses the default compiler settings like the
 * `printf` module.
 *
 * The fourth argument of the format engine is a pointer the run-time uses for
 * the output; the raw value `0x3d319` is a relocated placeholder.  The rebuild
 * passes the address of the harness `__iob` table, and the difference is a
 * link-time address that the diff accepts as an operand difference.
 */
#include <stdio.h>
#include <stdarg.h>

extern int __vsprintf(char *buffer, const char *format, void *args, void *file);

int sprintf(char *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = __vsprintf(buffer, format, &args, &__iob[0]);
    buffer[ret] = 0;
    return ret;
}
