/* The rebuilt `palette_shade_build` module.
 *
 * The original sits at linear 0x258d0 in STARTH.EXE and holds 151 instructions
 * and 475 bytes up to the `ret` at 0x25aaa.  The next function starts at
 * 0x25aac, so the store record keeps the 476-byte extent and sets no function
 * size.  The identity check uses 151 instructions.
 *
 * The five direct callers pass the palette at 0xcf5c in `eax` after a palette
 * load: race_session_start at 0x12a85, loading_screen at 0x2a44a and 0x2a638,
 * and name_entry_screen at 0x2c90b and 0x2c9fe.
 *
 * The function builds two 256-byte lookup tables.  The first table uses the
 * scale 0.75 and the second table uses the scale 0.6.  For every palette entry
 * it scales the red, green, and blue bytes, clamps each value at 255.0, and
 * finds the nearest palette entry by the squared distance.  It stores the
 * nearest index in the table.  The table pointer cells DAT_0015d1c and
 * DAT_000bcddc take the two table addresses 0x1cc00 and 0x1cd00.
 *
 * The pixel remap at 0xccd9a reads the DAT_000bcddc table and the block at
 * 0x25d53 reads the DAT_0015d1c table, so the tables are the shade maps of the
 * framebuffer.  The two scales dim the palette to 75 and 60 percent.
 *
 * The local scale array also holds 0.5 at 0x3f000000, but the loop bound 2
 * leaves the third value unused.
 *
 * The function converts only an integer to a float, so it needs no truncating
 * __CHP stub.  The shared fpu_stub.asm supplies the FPU run-time symbols and
 * the float-used marker.  The clamp constant arrives through the global
 * `palette_clamp`, because the minimal image does not load the constant
 * segment.  The original has the EBP frame, so the module builds with `-of+`.
 */
extern unsigned char DAT_001cc00[];
extern unsigned char *DAT_0015d1c;
extern unsigned char *DAT_000bcddc;
extern float palette_clamp;

void palette_shade_build(unsigned char *palette)
{
    float scales[3];
    float scale;
    float ref_r;
    float ref_g;
    float ref_b;
    float best_distance;
    float distance;
    float dr;
    float dg;
    float db;
    unsigned char *source;
    unsigned char *entry;
    int plane;
    int index;
    int candidate;
    int best_index;

    scales[0] = 0.75f;
    scales[1] = 0.6f;
    scales[2] = 0.5f;
    DAT_0015d1c = DAT_001cc00;
    DAT_000bcddc = DAT_001cc00 + 0x100;
    for (plane = 0; plane < 2; plane++) {
        scale = scales[plane];
        source = palette;
        for (index = 0; index < 0x100; index++) {
            best_distance = 22100000.0f;
            entry = palette;
            ref_r = (float)source[0] * scale;
            if (ref_r > palette_clamp) {
                ref_r = palette_clamp;
            }
            ref_g = (float)source[1] * scale;
            if (ref_g > palette_clamp) {
                ref_g = palette_clamp;
            }
            ref_b = (float)source[2] * scale;
            if (ref_b > palette_clamp) {
                ref_b = palette_clamp;
            }
            source = source + 3;
            best_index = 0;
            for (candidate = 0; candidate < 0x100; candidate++) {
                dr = (float)entry[0] - ref_r;
                dg = (float)entry[1] - ref_g;
                db = (float)entry[2] - ref_b;
                entry = entry + 3;
                distance = dr * dr + dg * dg + db * db;
                if (distance < best_distance) {
                    best_index = candidate;
                    best_distance = distance;
                }
            }
            DAT_001cc00[(plane << 8) + index] = (unsigned char)best_index;
        }
    }
}
