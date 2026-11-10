/* The test harness for the rebuilt `path_playback_apply` module.
 *
 * The harness holds the playback block, the playback state, the object table at
 * `DAT_0010653c`, four display objects, the enable flag, and the two float
 * constants.  It runs five paths:
 *
 *     0  the full path: slot 2 takes the position, the three scaled angles,
 *        and the two copied fields
 *     1  the enable flag is zero: the function writes nothing
 *     2  the state flag is zero: the function writes nothing
 *     3  the negative and the large angles: the conversions truncate toward
 *        zero and the low 11 bits mask the results
 *     4  the state flag is -1 and the slot is 1: the function still writes,
 *        and the writes follow the selected slot
 *     D  the exit marker
 *
 * The float values are set from their exact bit patterns, so the harness does
 * not depend on a float literal or on the constant segment.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void path_playback_apply(char *view);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static unsigned char view[0x40];
static unsigned char state[0x20];
static unsigned char body0[0x100];
static unsigned char body1[0x100];
static unsigned char body2[0x100];
static unsigned char body3[0x100];

unsigned char DAT_0010653c[0x100];
int DAT_0011d264;
float DAT_00100dc8;
float DAT_00100dcc;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_view_int(int offset, int value)
{
    *(int *)(view + offset) = value;
}

static void set_view_float(int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *(float *)(view + offset) = v.f;
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

static void set_entry(int index, unsigned char *body)
{
    *(unsigned char **)(DAT_0010653c + index * 28) = body;
}

static int int_is(void *block, int offset, int value)
{
    return *(int *)((char *)block + offset) == value;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x40; index++) {
        view[index] = 0;
    }
    for (index = 0; index < 0x20; index++) {
        state[index] = 0;
    }
    for (index = 0; index < 0x100; index++) {
        body0[index] = 0;
        body1[index] = 0;
        body2[index] = 0;
        body3[index] = 0;
        DAT_0010653c[index] = 0;
    }
    set_global_float(&DAT_00100dc8, 0x44800000);
    set_global_float(&DAT_00100dcc, 0x40490fdb);
    set_entry(0, body0);
    set_entry(1, body1);
    set_entry(2, body2);
    set_entry(3, body3);
    DAT_0011d264 = 1;
    *(unsigned char **)(view + 8) = state;
    *(int *)(state + 0xc) = 1;
    set_view_int(0, 2);
    set_view_int(0x10, 100);
    set_view_int(0x14, 200);
    set_view_int(0x18, 300);
    set_view_float(0x1c, 0x40400000);
    set_view_float(0x20, 0x40800000);
    set_view_float(0x24, 0x40c00000);
    set_view_int(0x28, 77);
    set_view_int(0x2c, 100);
}

int main(void)
{
    reset();
    path_playback_apply((char *)view);
    put(int_is(body2, 8, 100)
        && int_is(body2, 0xc, 200)
        && int_is(body2, 0x10, 300)
        && int_is(body2, 0x14, 0x3d1)
        && int_is(body2, 0x18, 0x517)
        && int_is(body2, 0x1c, 0x7a3)
        && int_is(body2, 0x20, 77)
        && int_is(body2, 0x3c, 100)
        && int_is(body0, 8, 0)
        && int_is(body1, 8, 0)
        && int_is(body3, 8, 0) ? '0' : 'x');

    reset();
    DAT_0011d264 = 0;
    path_playback_apply((char *)view);
    put(int_is(body2, 8, 0)
        && int_is(body2, 0x14, 0)
        && int_is(body2, 0x20, 0)
        && int_is(body2, 0x3c, 0) ? '1' : 'x');

    reset();
    *(int *)(state + 0xc) = 0;
    path_playback_apply((char *)view);
    put(int_is(body2, 8, 0)
        && int_is(body2, 0x14, 0)
        && int_is(body2, 0x20, 0)
        && int_is(body2, 0x3c, 0) ? '2' : 'x');

    reset();
    set_view_float(0x1c, 0xc0400000);
    set_view_float(0x20, 0x42c80000);
    set_view_float(0x24, 0x40000000);
    path_playback_apply((char *)view);
    put(int_is(body2, 0x14, 0x42f)
        && int_is(body2, 0x18, 0x752)
        && int_is(body2, 0x1c, 0x28b) ? '3' : 'x');

    reset();
    *(int *)(state + 0xc) = -1;
    set_view_int(0, 1);
    path_playback_apply((char *)view);
    put(int_is(body1, 8, 100)
        && int_is(body1, 0x14, 0x3d1)
        && int_is(body1, 0x18, 0x517)
        && int_is(body1, 0x20, 77)
        && int_is(body1, 0x3c, 100)
        && int_is(body2, 8, 0)
        && int_is(body0, 8, 0)
        && int_is(body3, 8, 0) ? '4' : 'x');

    put('D');
    quit();
    return 0;
}
