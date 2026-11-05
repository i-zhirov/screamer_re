/* The rebuilt `fprintf` module.
 *
 * The original sits at linear 0x4bf64 in STARTH.EXE and holds 13 instructions:
 * the register-save prologue `push ebx` and `push edx`, a 4-byte local, the
 * `va_start` address of the first variable argument, the stream and the format
 * arguments from the stack, the print-engine call at `0xd429f`, and the
 * epilogue.  The record that earlier named this function `dpmi_real_copy` is a
 * misnomer: the call sites are the message stubs of `error_report` and the
 * stream functions, and the shape matches `printf` exactly.
 *
 * The OpenWatcom V2 run-time source `bld/clib/streamio/c/fprintf.c` holds the
 * same shape:
 *
 *     int fprintf( FILE *stream, const char *format, ... )
 *     {
 *         va_list args;
 *         int ret;
 *         va_start( args, format );
 *         ret = __fprtf( stream, format, args );
 *         va_end( args );
 *         return( ret );
 *     }
 *
 * The V2 `va_list` is a plain pointer, so this rebuild passes the address of
 * the variable to reproduce the original form, which passed the `va_list` by
 * reference, the same way the `printf` module does.
 */
#include <stdio.h>
#include <stdarg.h>

extern int __fprtf(FILE *fp, const char *format, void *args);

int fprintf(FILE *stream, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    return __fprtf(stream, format, &args);
}
