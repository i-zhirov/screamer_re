/* The rebuilt `printf` module.
 *
 * The original sits at linear 0x3009b in STARTH.EXE and holds 13 instructions: the
 * register-save prologue `push ebx` and `push edx`, a 4-byte local, the `va_start`
 * address of the first variable argument, the address of the standard-output file, the
 * format argument, the print-engine call at 0xd429f, and the epilogue.
 *
 * The OpenWatcom V2 run-time source `bld/clib/streamio/c/printf.c` holds the same
 * shape:
 *
 *     int printf( const char *format, ... )
 *     {
 *         va_list args;
 *         int ret;
 *         va_start( args, format );
 *         ret = __fprtf( stdout, format, args );
 *         va_end( args );
 *         return( ret );
 *     }
 *
 * The V2 header defines `stdout` as `((__w_FILE *)&__iob[1])` and `va_start` as an
 * intrinsic, so the compiler emits the address of the standard-output entry.  The V2
 * `va_list` is a plain pointer, so this rebuild passes the address of the variable to
 * reproduce the original form, which passed the `va_list` by reference.  The `va_end`
 * call is absent, because the original epilogue holds no trace of it.
 */
#include <stdio.h>
#include <stdarg.h>

extern int __fprtf(FILE *fp, const char *format, void *args);

int printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    return __fprtf(stdout, format, &args);
}
