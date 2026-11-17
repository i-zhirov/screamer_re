/* The rebuilt `lzw_init` module.
 *
 * The original sits at linear 0x4d360 in STARTH.EXE and holds 32 instructions
 * and 153 bytes up to the `ret` at 0x4d3f8.  The next function `lzw_expand`
 * starts at 0x4d3fc, so the store record keeps the 156-byte extent and sets no
 * separate function size.  The identity check uses 32 instructions.
 *
 * The only direct caller is `member_open` at 0x4c128.  That function allocates
 * the 0x8490-byte decoder context through `memory_alloc` at 0x30050, loads the
 * context in `eax`, and calls this function at 0x4c2e3.  The same decoder
 * layer holds `lzw_expand` at 0x4d3fc, `lzw_block` at 0x4d4c4, and
 * `lzw_bit_read` at 0x4d844.  These facts support the name `lzw_init`, and the
 * record takes the section `The graphics decoders`.  The original has no
 * symbol, so the record keeps the name with the confidence inferred.
 *
 * The context arrives in `eax` and returns in `eax`.  The function writes the
 * four width masks 0x1ff, 0x3ff, 0x7ff, and 0xfff at `+0x8074`, `+0x8078`,
 * `+0x807c`, and `+0x8080`.  It clears the character stack count at `+0x4064`,
 * copies that count to the bit position at `+0x406c`, clears the state at
 * `+0x404c`, sets the code width at `+0x4068` to 9, the width threshold at
 * `+0x4060` to 0x200, and the next free entry at `+0x405c` to 0x102.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * function holds no floating-point instruction and no call, so the module
 * needs no FPU stub and no assembly.
 */

char *lzw_init(char *context)
{
    *(int *)(context + 0x8074) = 0x1ff;
    *(int *)(context + 0x8078) = 0x3ff;
    *(int *)(context + 0x807c) = 0x7ff;
    *(int *)(context + 0x8080) = 0xfff;
    *(int *)(context + 0x4064) = 0;
    *(int *)(context + 0x406c) = *(int *)(context + 0x4064);
    *(int *)(context + 0x404c) = 0;
    *(int *)(context + 0x4068) = 9;
    *(int *)(context + 0x4060) = 0x200;
    *(int *)(context + 0x405c) = 0x102;
    return context;
}
