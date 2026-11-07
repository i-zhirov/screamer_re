/* The rebuilt `background_scale` module.
 *
 * The original sits at linear 0x83ca8 in STARTH.EXE and holds 111 instructions
 * and 323 bytes up to the `ret` at 0x83dea.  The next record starts at
 * 0x843a8, so the store record keeps the 1792-byte extent and sets no separate
 * function size.  The identity check uses 111 instructions.
 *
 * The only direct caller is the update at 0x81ab0.  It calls this function at
 * the start of the frame, before the source update and the mix.  The material
 * loader at 0x4cd9c decodes the background member `%s.BCK` into the buffer at
 * `DAT_000214e8`, so the record takes the name `background_scale` with the
 * confidence inferred.
 *
 * The function renders the decoded background into the video buffer:
 *
 *     offset  form the source row offset from the angle at 0x724cc and the
 *             pitch at 0x7a7dc, and store it in 0x73f10
 *     column  form the source column offset from the camera field at +4 and
 *             store it in 0x73dec
 *     start   select the source row from a negative offset or a wrapped
 *             offset, and add the column offset
 *     rows    write each source row twice; each source byte becomes one
 *             16-bit value with both halves equal
 *     fill    clear the rest of the video buffer up to the row 0x7a7c4 with
 *             the color at 0x7a7bc
 *
 * The original saves all registers with `pushal` and restores them with
 * `popal`.  It has no EBP frame.  The function uses no floating-point
 * instruction and calls no function.
 *
 * The minimal image needs one initialized data byte, because the DOS4G loader
 * leaves the BSS object unmapped when the data object holds no page.
 */

extern int DAT_000724cc;
extern int DAT_0007a7dc;
extern int *DAT_0007a7ac;
extern int DAT_0007a7c4;
extern int DAT_0007a7bc;
extern unsigned char *DAT_000214e8;
extern unsigned char *DAT_00121528;
extern int DAT_00073f10;
extern int DAT_00073dec;

/* Keep one initialized data byte in the image.  The DOS4G loader leaves the
 * BSS object unmapped when the data object holds no page. */
int background_scale_marker = 0;

void background_scale(void)
{
    int offset;
    int column;
    int span;
    int row;
    int shifted;
    int count;
    unsigned char *source;
    unsigned char *write;
    int fill;
    int index;

    offset = (DAT_000724cc * -5000) >> 7;
    offset /= -5000;
    offset += DAT_0007a7dc + 0x28;
    offset -= 0x200;
    offset += 2;
    DAT_00073f10 = offset;

    column = (0x7ff - DAT_0007a7ac[1]) >> 1;
    column &= 0x1ff;
    DAT_00073dec = column;

    span = 0x200;
    row = offset;
    shifted = row;
    if (row < 0) {
        span += row;
        shifted = -shifted;
    }
    source = DAT_000214e8 + (shifted >> 1) * 0x200;
    row += 0x200;
    if (row > DAT_0007a7c4) {
        row -= DAT_0007a7c4;
        span -= row;
    }
    source += column;
    write = DAT_00121528;

    count = span >> 1;
    while (--count > 0) {
        for (index = 0; index < 0x140; index++) {
            unsigned char value;

            value = source[index];
            write[index * 2] = value;
            write[index * 2 + 1] = value;
        }
        write += 0x280;
        for (index = 0; index < 0x140; index++) {
            unsigned char value;

            value = source[index];
            write[index * 2] = value;
            write[index * 2 + 1] = value;
        }
        write += 0x280;
        source += 0x200;
    }

    fill = (int)(DAT_00121528 + (DAT_0007a7c4 + 1) * 0x280 - write);
    fill >>= 2;
    fill += 0xa0;
    for (index = 0; index < fill; index++) {
        ((unsigned int *)write)[index] = DAT_0007a7bc;
    }
}
