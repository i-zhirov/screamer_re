/* The rebuilt `__fprtf` run-time module.
 *
 * The original sits at linear 0xd429f in STARTH.EXE and holds 52 instructions
 * and 133 bytes: the three register saves, the file flag save, the 0x30-bit
 * clear, the buffer check and the allocate call at `0xddda9`, the 0xd flag
 * change, the format call at `0xd68c0`, the restore call at `0xddfa8`, the
 * error test, the flag restore, and the epilogue.
 *
 * The function is the stream formatter that `printf` and `fprintf` call at
 * `0x3009b` and `0x4bf64`.  The earlier record named it `dpmi_issue_int`; the
 * call sites and the flag work show the run-time `__fprtf` role.
 *
 * The inner format call receives the file in `eax`, the format in `edx`, the
 * argument list in `ebx`, and the table at `DAT_001c428e` in `ecx`.
 */
extern char DAT_001c428e[];
extern void stream_alloc(void *file);
extern int __doformat(void *file, char *format, void *args, void *table);
extern void stream_restore(void *file);

int __fprtf(void *file, char *format, void *args)
{
    unsigned int flags;
    unsigned int saved;
    int changed;
    int result;

    flags = *(unsigned int *)((char *)file + 0xc);
    saved = flags & 0x30;
    *(unsigned char *)((char *)file + 0xc) = (unsigned char)(flags & 0xcf);
    if (*(void **)((char *)file + 8) == 0) {
        stream_alloc(file);
    }
    changed = 0;
    if ((*(unsigned char *)((char *)file + 0xd) & 4) != 0) {
        *(unsigned char *)((char *)file + 0xd) &= 0xfa;
        *(unsigned char *)((char *)file + 0xd) |= 1;
        changed = 1;
    }
    result = __doformat(file, format, args, DAT_001c428e);
    if (changed) {
        *(unsigned char *)((char *)file + 0xd) &= 0xfa;
        *(unsigned char *)((char *)file + 0xd) |= 4;
        stream_restore(file);
    }
    if ((*(unsigned char *)((char *)file + 0xc) & 0x20) != 0) {
        result = -1;
    }
    *(unsigned int *)((char *)file + 0xc) |= saved;
    return result;
}
