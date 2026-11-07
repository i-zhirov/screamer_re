/* The test harness for the rebuilt `traffic_object_link` module.
 *
 * The harness holds one 0x300-byte traffic record, the four entry pairs of the
 * 28-byte table at `DAT_0010653c`, the body and render blocks, and the two
 * constant globals.  It runs six paths:
 *
 *     0  the full path: the body and the part take the position, the five
 *        scaled angles, the record value at 0x3c, and the masked flag
 *     1  the two negative floats: the conversions truncate toward zero
 *     2  the large and the negative angle: the low 11 bits mask the results
 *     3  the record field +0x14 is 0xabcd: only the high nibble survives
 *     4  the record fields +0x64 and +0x6c select the entries 3 and 0: the
 *        writes follow the selected body and part
 *     5  the part takes only its own fields: the body flag and value do not
 *        reach the part
 *     6  the first record field is -1: the function writes nothing
 *     D  the exit marker
 *
 * The float values are set from their exact bit patterns, so the harness does
 * not depend on a float literal or on the constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void traffic_object_link(char *record);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char record[0x300];
static unsigned char body0[0x100];
static unsigned char body1[0x100];
static unsigned char body2[0x100];
static unsigned char body3[0x100];
static unsigned char render0[0x40];
static unsigned char render1[0x40];
static unsigned char render2[0x40];
static unsigned char render3[0x40];

unsigned char DAT_0010653c[0x100];
float DAT_00100bc4;
float DAT_00100bc8;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_record_int(int offset, int value)
{
    *(int *)(record + offset) = value;
}

static void set_record_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(record + offset) = v.f;
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

static void set_entry(int index, char *body, char *render)
{
    *(char **)(DAT_0010653c + index * 28) = body;
    *(char **)(DAT_0010653c + index * 28 + 4) = render;
}

static int int_is(void *block, int offset, int value)
{
    return *(int *)((char *)block + offset) == value;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x300; index++) {
        record[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        body0[index] = 0;
        body1[index] = 0;
        body2[index] = 0;
        body3[index] = 0;
        DAT_0010653c[index] = 0;
    }
    for (index = 0; index < 0x40; index++) {
        render0[index] = 0;
        render1[index] = 0;
        render2[index] = 0;
        render3[index] = 0;
    }
    set_global_float(&DAT_00100bc4, 0x44800000);
    set_global_float(&DAT_00100bc8, 0x40490fdb);
    set_entry(0, (char *)body0, (char *)render0);
    set_entry(1, (char *)body1, (char *)render1);
    set_entry(2, (char *)body2, (char *)render2);
    set_entry(3, (char *)body3, (char *)render3);
    set_record_int(0x64, 1);
    set_record_int(0x6c, 2);
    set_record_int(0x14, 0x5000);
    set_record_int(0x18, 77);
    set_record_int(0x3c, 100);
    set_record_int(0x40, 200);
    set_record_int(0x44, 300);
    set_record_int(0x54, 555);
    set_record_float(0x48, 0x40400000);
    set_record_float(0x4c, 0x40800000);
    set_record_float(0x50, 0x40c00000);
    set_record_float(0x58, 0x40000000);
    set_record_float(0x5c, 0x3fc00000);
}

int main(void)
{
    reset();
    traffic_object_link((char *)record);
    put(int_is(body1, 8, 100)
        && int_is(body1, 0xc, 200)
        && int_is(body1, 0x10, 300)
        && int_is(body1, 0x14, 0x3d1)
        && int_is(body1, 0x18, 0x517)
        && int_is(body1, 0x1c, 0x7a3)
        && int_is(body1, 0x3c, 77)
        && int_is(body1, 0x20, 0x5000)
        && int_is(body2, 8, 100)
        && int_is(body2, 0xc, 555)
        && int_is(body2, 0x10, 300)
        && int_is(body2, 0x14, 0x28b)
        && int_is(body2, 0x18, 0x517)
        && int_is(body2, 0x1c, 0x1e8)
        && int_is(body2, 0x20, 0)
        && int_is(body2, 0x3c, 0)
        && int_is(body0, 8, 0)
        && int_is(body3, 8, 0) ? '0' : 'x');

    reset();
    set_record_float(0x48, 0xc0400000);
    set_record_float(0x58, 0xc0000000);
    traffic_object_link((char *)record);
    put(int_is(body1, 0x14, 0x42f)
        && int_is(body2, 0x14, 0x575) ? '1' : 'x');

    reset();
    set_record_float(0x4c, 0xc2c80000);
    set_record_float(0x50, 0x42c80000);
    set_record_float(0x5c, 0x42c80000);
    traffic_object_link((char *)record);
    put(int_is(body1, 0x18, 0xae)
        && int_is(body1, 0x1c, 0x752)
        && int_is(body2, 0x1c, 0x752) ? '2' : 'x');

    reset();
    set_record_int(0x14, 0xabcd);
    traffic_object_link((char *)record);
    put(int_is(body1, 0x20, 0xa000) ? '3' : 'x');

    reset();
    set_record_int(0x64, 3);
    set_record_int(0x6c, 0);
    traffic_object_link((char *)record);
    put(int_is(body3, 8, 100)
        && int_is(body3, 0x14, 0x3d1)
        && int_is(body0, 8, 100)
        && int_is(body0, 0x10, 300)
        && int_is(body0, 0x18, 0x517)
        && int_is(body1, 8, 0)
        && int_is(body2, 8, 0) ? '4' : 'x');

    reset();
    set_record_int(0x14, 0x7000);
    traffic_object_link((char *)record);
    put(int_is(body1, 0x20, 0x7000)
        && int_is(body2, 0xc, 555)
        && int_is(body1, 0xc, 200)
        && int_is(body2, 0x20, 0)
        && int_is(body2, 0x3c, 0) ? '5' : 'x');

    reset();
    set_record_int(0, -1);
    traffic_object_link((char *)record);
    put(int_is(body1, 8, 0)
        && int_is(body1, 0x14, 0)
        && int_is(body1, 0x20, 0)
        && int_is(body1, 0x3c, 0)
        && int_is(body2, 8, 0)
        && int_is(body2, 0xc, 0) ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
