/* The rebuilt `video_present` module.
 *
 * The original sits at linear 0x27ec8 in STARTH.EXE and holds 29 instructions
 * and 124 bytes: the three register saves, the frame, the 0x310-byte local
 * area, the 0x300-byte palette loop, the scale and offset arithmetic, the
 * rounding helper at `0x4d9f4`, the palette call at `0x81a05`, and the
 * `ret 8` epilogue.
 *
 * The function maps the 0x300 source bytes at `DAT_0010cf5c` with the formula
 * `(byte - offset) * scale + offset`, stores the result in the local palette,
 * and passes it to `palette_set`.  The two arguments arrive on the stack in the
 * original; the rebuild uses the compiler convention and the same arithmetic.
 */
extern unsigned char DAT_0010cf5c[];
extern void palette_set(char *palette);

void video_present(float scale, int offset)
{
    char palette[0x300];
    int index;
    int value;

    for (index = 0; index < 0x300; ++index) {
        value = (int)DAT_0010cf5c[index] - offset;
        palette[index] = (char)((int)((float)value * scale + offset));
    }
    palette_set(palette);
}
