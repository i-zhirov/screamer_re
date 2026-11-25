/* The interim compiler and run-time label cluster.
 *
 * The whole-image link reports eight compiler or run-time symbols as
 * undefined.  The rebuilt modules and CLIB3R reference them, but the shipped
 * definitions sit in the original code and in the math libraries that the
 * image does not link:
 *
 *   __vsprintf_   the in-image format engine at 0xd68c0, called by sprintf
 *   __doformat_   the same engine, called by __fprtf
 *   __cnvs2d_     the numeric conversion helper of the EFG format support
 *   _EFG_Format_  the EFG format table of the math library
 *   __LpDllName   the profiler DLL-name pointer of CLIB3R
 *   __int7        the interrupt-7 handler of the FPU emulator
 *   __int7_pl3    the protected-mode interrupt-7 handler
 *   main_         the C main entry that the startup jumps to
 *
 * The V2 C startup chain then exposes one more label, `__DLLstart_`, the DLL
 * startup entry of `dllmncwd.c`.  The game is an EXE, so the entry is not
 * called.
 *
 * The two format entry points call the rebuilt formatter at 0xd68c0.  The
 * string entry passes the string sink, and the stream entry passes the stream
 * sink.  The conversion helper still returns zero, the EFG table holds 256
 * zero bytes, and the profiler pointer stays null; the exact signature of
 * `__cnvs2d` and the byte size of the EFG table are unknown.  The main entry
 * calls the rebuilt main invoker, which aligns the stack, grows the stack,
 * calls the game main, and terminates.
 *
 * The symbol names follow the Watcom conventions: a default C function gets a
 * trailing underscore, a data object gets a leading underscore, and an
 * identifier that starts with an underscore keeps it.  The interrupt handlers
 * and the DLL entry are in `compiler_runtime.asm`, because their linker names
 * are raw.
 *
 * The module has no function record and no behavior check, so it does not
 * enter the verified count.
 */
#define EFG_TABLE_SIZE 0x100

typedef void (*formatter_sink)(void *out, char ch);

extern void main_invoker(void);
extern int formatter(void *out, const char *format, void *args,
    formatter_sink sink);
extern void formatter_string_sink(void *out, char ch);
extern void formatter_stream_sink(void *out, char ch);

char *_LpDllName;
unsigned char EFG_Format_[EFG_TABLE_SIZE];

int __vsprintf(char *buffer, const char *format, void *args, void *file)
{
    (void)file;
    return formatter(buffer, format, args, formatter_string_sink);
}

int __doformat(void *file, char *format, void *args, void *table)
{
    (void)table;
    return formatter(file, format, args, formatter_stream_sink);
}

int __cnvs2d(void)
{
    return 0;
}

int main(void)
{
    main_invoker();
    return 0;
}
