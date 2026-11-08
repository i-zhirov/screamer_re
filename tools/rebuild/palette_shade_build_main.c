/* The test harness for the rebuilt `palette_shade_build` module.
 *
 * The harness holds the 0x300-byte palette and the 0x200-byte table region at
 * `DAT_001cc00`.  It runs five paths:
 *
 *     0  the grayscale palette: the two pointer cells take the table addresses,
 *        and the scale 0.75 table holds the sampled nearest indices
 *     1  the grayscale palette: the scale 0.6 table holds the sampled nearest
 *        indices
 *     2  the colored palette: the source entry 10 scales to the entry 11 at
 *        0.75 and to the entry 12 at 0.6
 *     3  the grayscale palette: the scale 0.75 table does not decrease and its
 *        maximum is 191
 *     4  the grayscale palette: the scale 0.6 table stays at or below the scale
 *        0.75 table for every entry
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void palette_shade_build(unsigned char *palette);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned char DAT_001cc00[0x200];
unsigned char *DAT_0015d1c;
unsigned char *DAT_000bcddc;
float palette_clamp;

static unsigned char palette[0x300];
static char text[8];

static void set_bits_float(float *value, unsigned int pattern)
{
    *(unsigned int *)value = pattern;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset_tables(void)
{
    int index;

    for (index = 0; index < 0x200; index++) {
        DAT_001cc00[index] = 0;
    }
    DAT_0015d1c = 0;
    DAT_000bcddc = 0;
}

static void reset_gray(void)
{
    int index;

    for (index = 0; index < 0x100; index++) {
        palette[index * 3] = (unsigned char)index;
        palette[index * 3 + 1] = (unsigned char)index;
        palette[index * 3 + 2] = (unsigned char)index;
    }
    reset_tables();
}

static void reset_color(void)
{
    reset_gray();
    palette[10 * 3] = 100;
    palette[10 * 3 + 1] = 0;
    palette[10 * 3 + 2] = 0;
    palette[11 * 3] = 75;
    palette[11 * 3 + 1] = 0;
    palette[11 * 3 + 2] = 0;
    palette[12 * 3] = 60;
    palette[12 * 3 + 1] = 0;
    palette[12 * 3 + 2] = 0;
}

static unsigned char table0(int index)
{
    return DAT_001cc00[index];
}

static unsigned char table1(int index)
{
    return DAT_001cc00[0x100 + index];
}

int main(void)
{
    int index;
    int ok;

    set_bits_float(&palette_clamp, 0x437f0000);
    reset_gray();
    palette_shade_build(palette);
    ok = DAT_0015d1c == DAT_001cc00
        && DAT_000bcddc == DAT_001cc00 + 0x100
        && table0(0) == 0
        && table0(1) == 1
        && table0(2) == 1
        && table0(3) == 2
        && table0(4) == 3
        && table0(100) == 75
        && table0(254) == 190
        && table0(255) == 191;
    put(ok ? '0' : 'x');

    reset_gray();
    palette_shade_build(palette);
    ok = table1(0) == 0
        && table1(1) == 1
        && table1(2) == 1
        && table1(3) == 2
        && table1(4) == 2
        && table1(100) == 60
        && table1(254) == 152
        && table1(255) == 153;
    put(ok ? '1' : 'x');

    reset_color();
    palette_shade_build(palette);
    ok = table0(10) == 11 && table1(10) == 12;
    put(ok ? '2' : 'x');

    reset_gray();
    palette_shade_build(palette);
    ok = DAT_001cc00[0] == 0;
    for (index = 1; index < 0x100; index++) {
        ok = ok && table0(index) >= table0(index - 1);
    }
    ok = ok && table0(0xff) == 191;
    put(ok ? '3' : 'x');

    reset_gray();
    palette_shade_build(palette);
    ok = 1;
    for (index = 0; index < 0x100; index++) {
        ok = ok && table1(index) <= table0(index);
    }
    put(ok ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
