/* The rebuilt `lzw_block` module.
 *
 * The original sits at linear 0x4d4c4 in STARTH.EXE and holds 73 instructions
 * and 290 bytes up to the first `ret` at 0x4d5e5.  The function has further
 * exits at the `ret` at 0x4d79d and the `ret` at 0x4d843.  The next function
 * starts at 0x4d844, so the store record keeps the 896-byte extent and sets no
 * separate function size.  The identity check uses 73 instructions.
 *
 * The only direct caller is the expander at 0x4d3fc, which loops on this
 * function until it returns 0.  The reference table in
 * docs/reference/file-formats.md names the address `lzw_block`, so the module
 * keeps the name with the confidence inferred and the section
 * `The graphics decoders`.
 *
 * The context holds the output buffer pointer at `+0x8488`, the output limit
 * at `+0x4048`, the state at `+0x404c`, the working code at `+0x4050`, the
 * previous code at `+0x4054`, the saved code at `+0x4058`, the next free
 * entry at `+0x405c`, the width threshold at `+0x4060`, the stack count at
 * `+0x4064`, the code width at `+0x4068`, the output position at `+0x4070`,
 * the character stack at `+0x4074`, and the two characters at `+0x8084` and
 * `+0x8085`.  The dictionary table starts at `+0x0000`; an entry holds a
 * 16-bit prefix at `+0x00` and a character at `+0x02`.
 *
 * A clear code 0x100 resets the width to 9, the threshold to 0x200, the next
 * entry to 0x102, reads the first code, and writes its character.  An end code
 * 0x101 returns 0.  Any other code walks the prefix chain, writes the string
 * to the output buffer, and adds the entry `previous code + first character`
 * at the next free slot.  The width grows when the next entry reaches the
 * threshold, up to 12.  When the output position reaches the limit in the
 * middle of a string, the state becomes 2 and the next call continues the
 * string.
 *
 * The original has the EBP frame, so the module builds with `-of+`.  The
 * original uses no floating-point instruction, so the module needs no FPU
 * stub.
 */
extern int lzw_bit_read(char *context);

int lzw_block(char *context)
{
    int state;
    int code;
    int value;
    int next;
    int position;

    state = *(int *)(context + 0x404c);
    *(int *)(context + 0x404c) = 0;
    if (state == 2) {
        goto drain;
    }
    code = lzw_bit_read(context);
    if (code == 0x101) {
        return 0;
    }
    if (code == 0x100) {
        *(int *)(context + 0x4068) = 9;
        *(int *)(context + 0x4060) = 0x200;
        *(int *)(context + 0x405c) = 0x102;
        *(int *)(context + 0x4050) = lzw_bit_read(context);
        *(int *)(context + 0x4054) = *(int *)(context + 0x4050);
        context[0x8085] = (char)(*(int *)(context + 0x4054));
        context[0x8084] = context[0x8085];
        position = *(int *)(context + 0x4070);
        *(int *)(context + 0x4070) = position + 1;
        *(char *)(*(char **)(context + 0x8488) + position) = context[0x8084];
        if (*(int *)(context + 0x4070) != *(int *)(context + 0x4048)) {
            return 1;
        }
        *(int *)(context + 0x404c) = 1;
        return 0;
    }

    *(int *)(context + 0x4050) = code;
    *(int *)(context + 0x4058) = *(int *)(context + 0x4050);
    if (code >= *(int *)(context + 0x405c)) {
        *(int *)(context + 0x4050) = *(int *)(context + 0x4054);
        value = *(int *)(context + 0x4050);
        *(char *)&value = context[0x8084];
        *(int *)(context + 0x4074 + *(int *)(context + 0x4064) * 4) = value;
        *(int *)(context + 0x4064) += 1;
    }
    while (*(int *)(context + 0x4050) > 0xff) {
        value = *(int *)(context + 0x4050);
        *(char *)&value =
            *(char *)(context + *(int *)(context + 0x4050) * 4 + 2);
        *(int *)(context + 0x4074 + *(int *)(context + 0x4064) * 4) = value;
        *(int *)(context + 0x4064) += 1;
        value = *(unsigned short *)(context + *(int *)(context + 0x4050) * 4);
        *(int *)(context + 0x4050) = value;
    }
    value = *(int *)(context + 0x4050);
    context[0x8084] = (char)(*(int *)(context + 0x4050));
    context[0x8085] = context[0x8084];
    *(int *)(context + 0x4074 + *(int *)(context + 0x4064) * 4) = value;
    *(int *)(context + 0x4064) += 1;

drain:
    if (*(int *)(context + 0x4064) == 0) {
        goto add;
    }
    *(int *)(context + 0x4064) -= 1;
    value = *(int *)(context + 0x4074 + *(int *)(context + 0x4064) * 4);
    position = *(int *)(context + 0x4070);
    *(int *)(context + 0x4070) = position + 1;
    *(char *)(*(char **)(context + 0x8488) + position) = (char)value;
    if (*(int *)(context + 0x4070) != *(int *)(context + 0x4048)) {
        goto drain;
    }
    *(int *)(context + 0x404c) = 2;
    return 0;

add:
    *(char *)(context + *(int *)(context + 0x405c) * 4 + 2) =
        context[0x8085];
    next = *(int *)(context + 0x405c);
    *(int *)(context + 0x405c) = next + 1;
    *(unsigned short *)(context + next * 4) =
        (unsigned short)(*(int *)(context + 0x4054));
    value = *(int *)(context + 0x4058);
    *(int *)(context + 0x4054) = value;
    if (*(int *)(context + 0x405c) < *(int *)(context + 0x4060)) {
        return 1;
    }
    if (*(int *)(context + 0x4068) == 0xc) {
        return 1;
    }
    *(int *)(context + 0x4068) += 1;
    *(int *)(context + 0x4060) = *(int *)(context + 0x4060) * 2;
    return 1;
}
