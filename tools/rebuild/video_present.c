/* The rebuilt `video_present` module.
 *
 * The original sits at linear 0x27ec8 in STARTH.EXE.  It walks the 0x300 source
 * bytes at `DAT_0010cf5c` with a source pointer and an end pointer, writes the
 * mapped byte through a destination pointer into the local palette, and passes
 * the palette to `palette_set`.  The pointer form is what the original compiler
 * emitted; an index loop folds the walk into a counter and changes the code.
 */
extern unsigned char DAT_0010cf5c[];
extern void palette_set(char *palette);

void video_present(float scale, int offset)
{
    char palette[0x300];
    unsigned char *source;
    unsigned char *end;
    char *destination;
    int value;

    source = DAT_0010cf5c;
    end = DAT_0010cf5c + 0x300;
    destination = palette;
    while (source < end) {
        value = (int)*source - offset;
        *destination = (char)((int)((float)value * scale + offset));
        ++source;
        ++destination;
    }
    palette_set(palette);
}
