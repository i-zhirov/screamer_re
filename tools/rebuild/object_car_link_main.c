/* The test harness for the rebuilt `object_car_link` module.
 *
 * The harness holds the object record, the three entry pairs of the 28-byte
 * table at `DAT_0010653c`, the six constant globals, and the flag table at
 * `DAT_0010d268`.  It runs seven paths:
 *
 *     0  the state at `+0x2f8` is set: the two masked values go to the link
 *        and the target, the five scaled values form, and the flags hold
 *        0x3000
 *     1  the state at `+0x2f8` is clear and the angle is 5000.0: the target
 *        copies the two link values and the angle bit 0x2000 stays off
 *     2  the field `+0x2c0` is 0: the flag bit 0x4000 sets
 *     3  the angle is 4000.0: the flag bit 0x2000 sets and the low bit clears
 *     4  the three negative values: the five conversions truncate toward zero
 *     5  the two large values: the low 11 bits mask the results
 *     6  the object fields `+0x4dc` and `+4` select the other entries: the
 *        writes follow the selected link and target
 *     D  the exit marker
 *
 * The float values are set from their exact bit patterns, so the harness does
 * not depend on a float literal or on the constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void object_car_link(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char object[0x600];
static unsigned char link0[0x100];
static unsigned char render0[0x200];
static unsigned char link1[0x100];
static unsigned char render1[0x200];
static unsigned char link2[0x100];
static unsigned char render2[0x200];

unsigned char DAT_0010653c[0x100];
unsigned char DAT_0010d268[0x400];
float DAT_00100af4;
float DAT_00100af8;
float DAT_00100afc;
float DAT_00100b00;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_object_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int object_int_is(int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static void set_object_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(object + offset) = v.f;
}

static void set_global_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static void set_entry(int index, char *link, char *render)
{
    *(char **)(DAT_0010653c + index * 28) = link;
    *(char **)(DAT_0010653c + index * 28 + 4) = render;
}

static void set_render_int(char *render, int index, int offset, int value)
{
    *(int *)(render + index * 32 + offset) = value;
}

static int render_int_is(char *render, int index, int offset, int value)
{
    return *(int *)(render + index * 32 + offset) == value;
}

static int link_int_is(char *link, int offset, int value)
{
    return *(int *)(link + offset) == value;
}

static void set_flag(int type, int value)
{
    *(int *)(DAT_0010d268 + type * 64) = value;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x600; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        link0[index] = 0;
        link1[index] = 0;
        link2[index] = 0;
        DAT_0010653c[index] = 0;
    }
    for (index = 0; index < 0x200; index++) {
        render0[index] = 0;
        render1[index] = 0;
        render2[index] = 0;
    }
    for (index = 0; index < 0x400; index++) {
        DAT_0010d268[index] = 0;
    }
    set_global_float(&DAT_00100af4, 0x44800000);
    set_global_float(&DAT_00100af8, 0x40490fdb);
    set_global_float(&DAT_00100afc, 0x41700000);
    set_global_float(&DAT_00100b00, 0x457a0000);
    set_entry(0, (char *)link0, (char *)render0);
    set_entry(1, (char *)link1, (char *)render1);
    set_entry(2, (char *)link2, (char *)render2);
    set_object_int(0x4dc, 1);
    set_object_int(4, 2);
    set_object_int(0x498, 100);
    set_object_int(0x49c, 200);
    set_object_int(0x4a0, 300);
    set_object_int(0x2c4, 77);
    set_object_int(0x4e8, 555);
    set_object_float(0x4a4, 0x40400000);
    set_object_float(0x4bc, 0x40800000);
    set_object_float(0x4ac, 0xbfc00000);
    set_object_float(0x4b0, 0x40000000);
    set_object_float(0x4b8, 0x3f800000);
    set_object_float(0x418, 0x41f00000);
    set_object_int(0x58, 1);
    set_object_int(0x2dc, 10);
    set_object_int(0x2d4, 11);
    set_object_int(0x2cc, 12);
    set_object_int(0x4e0, 13);
    set_object_int(0x100, 4);
    set_object_int(0xac, 5);
    set_object_int(0x154, 6);
    set_object_int(0x2f8, 1);
    set_object_int(0x2c0, 1);
    set_object_int(0x2e0, 3);
    set_flag(3, 0x1000);
    set_render_int((char *)render1, 1, 8, 1000);
}

int main(void)
{
    reset();
    object_car_link((char *)object);
    put(link_int_is((char *)link1, 8, 100)
        && link_int_is((char *)link1, 0xc, 200)
        && link_int_is((char *)link1, 0x10, 300)
        && link_int_is((char *)link1, 0x14, 0x3d1)
        && link_int_is((char *)link1, 0x18, 0x517)
        && link_int_is((char *)link1, 0x1c, 0x618)
        && link_int_is((char *)link1, 0x3c, 555)
        && link_int_is((char *)link1, 0x20, 0x3000)
        && link_int_is((char *)link2, 8, 100)
        && link_int_is((char *)link2, 0xc, 77)
        && link_int_is((char *)link2, 0x10, 300)
        && link_int_is((char *)link2, 0x18, 0x517)
        && render_int_is((char *)render1, 10, 8, 0x145)
        && render_int_is((char *)render1, 11, 8, 0x28b)
        && render_int_is((char *)render1, 11, 0x10, 0x145)
        && render_int_is((char *)render1, 12, 8, 0x28b)
        && render_int_is((char *)render1, 12, 0x10, 0x145)
        && render_int_is((char *)render1, 13, 8, 0x28b)
        && render_int_is((char *)render1, 13, 0x10, 0x145)
        && render_int_is((char *)render1, 1, 8, 0x3e6)
        && render_int_is((char *)render1, 4, 8, 0x3e6)
        && render_int_is((char *)render1, 5, 8, 0x3e6)
        && render_int_is((char *)render1, 6, 8, 0x3e6) ? '0' : 'x');

    reset();
    set_object_int(0x2f8, 0);
    set_object_float(0x418, 0x459c4000);
    object_car_link((char *)object);
    put(link_int_is((char *)link2, 0x14, 0x3d1)
        && link_int_is((char *)link2, 0x1c, 0x618)
        && link_int_is((char *)link1, 0x20, 0x1000)
        && render_int_is((char *)render1, 1, 8, 0x29a) ? '1' : 'x');

    reset();
    set_object_int(0x2c0, 0);
    set_flag(3, 8);
    object_car_link((char *)object);
    put(link_int_is((char *)link1, 0x20, 0x6008)
        && link_int_is((char *)link2, 0x14, 0)
        && link_int_is((char *)link2, 0x1c, 0) ? '2' : 'x');

    reset();
    set_object_float(0x418, 0x457a0000);
    set_flag(3, 8);
    object_car_link((char *)object);
    put(link_int_is((char *)link1, 0x20, 0x2008)
        && render_int_is((char *)render1, 1, 8, 0x2dc) ? '3' : 'x');

    reset();
    set_object_float(0x4a4, 0xbfc00000);
    set_object_float(0x4b0, 0xc0200000);
    set_render_int((char *)render1, 1, 8, 0);
    object_car_link((char *)object);
    put(link_int_is((char *)link1, 0x14, 0x618)
        && render_int_is((char *)render1, 11, 8, 0x4d2)
        && render_int_is((char *)render1, 1, 8, 0x7fe) ? '4' : 'x');

    reset();
    set_object_float(0x4a4, 0x42c80000);
    set_object_float(0x4b8, 0x42c80000);
    object_car_link((char *)object);
    put(link_int_is((char *)link1, 0x14, 0x752)
        && render_int_is((char *)render1, 10, 8, 0x752)
        && render_int_is((char *)render1, 11, 0x10, 0x752) ? '5' : 'x');

    reset();
    set_object_int(0x4dc, 0);
    set_object_int(4, 1);
    object_car_link((char *)object);
    put(link_int_is((char *)link0, 8, 100)
        && link_int_is((char *)link0, 0x18, 0x517)
        && render_int_is((char *)render0, 10, 8, 0x145)
        && link_int_is((char *)link1, 8, 100)
        && link_int_is((char *)link1, 0xc, 77)
        && link_int_is((char *)link1, 0x18, 0x517)
        && link_int_is((char *)link2, 8, 0)
        && render_int_is((char *)render1, 1, 8, 1000) ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
