/* The test harness for the rebuilt `body_point_place` module.
 *
 * The harness holds one car record with the two local body points at `+0x41c`
 * and `+0x428`, the heading at `+0x4a8`, the world position at `+0x498` and
 * `+0x4a0`, and the two world body points at `+0x434` and `+0x440`.  The
 * rotation stub records the output pointer, the input pointer, and the angle of
 * every call, and it writes a chosen result for each call.
 *
 * The harness runs six paths:
 *
 *     0  the two calls: the output and the input pointers, the negated heading
 *        1.5, the two world points after the world offset, and the untouched y
 *        fields and input points
 *     1  the heading +0.0: the negation gives the bit pattern 0x80000000
 *     2  the heading -0.0: the negation gives the bit pattern 0x00000000
 *     3  the heading -2.0: the negation gives 2.0
 *     4  the two world points: the x offset `+0x498` and the z offset `+0x4a0`
 *        reach both points, and the second call takes the second input point
 *     5  the heading 2.5: the negation gives -2.5
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void body_point_place(char *self);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct rotate_point {
    int x;
    int pad;
    int z;
};

static char text[8];
static char object[0x900];
static int rotate_calls;
static int rotate_out_addr[8];
static int rotate_in_addr[8];
static unsigned int rotate_angle[8];
static int rotate_out_x[8];
static int rotate_out_z[8];

void rotate_pair(struct rotate_point *out, struct rotate_point *in,
    float angle)
{
    rotate_out_addr[rotate_calls] = (int)out;
    rotate_in_addr[rotate_calls] = (int)in;
    rotate_angle[rotate_calls] = *(unsigned int *)&angle;
    out->x = rotate_out_x[rotate_calls];
    out->z = rotate_out_z[rotate_calls];
    rotate_calls = rotate_calls + 1;
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

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static void set_bits(char *base, int offset, unsigned int bits)
{
    *(unsigned int *)(base + offset) = bits;
}

static void reset(void)
{
    int index;

    clear_bytes(object, 0x900);
    rotate_calls = 0;
    for (index = 0; index < 8; index++) {
        rotate_out_addr[index] = 0;
        rotate_in_addr[index] = 0;
        rotate_angle[index] = 0;
        rotate_out_x[index] = 0;
        rotate_out_z[index] = 0;
    }
    set_bits(object, 0x4a8, 0x3fc00000);
    set_int(object, 0x498, 1000);
    set_int(object, 0x4a0, -2000);
    set_int(object, 0x41c, 0x111);
    set_int(object, 0x424, 0x112);
    set_int(object, 0x428, 0x221);
    set_int(object, 0x430, 0x222);
    set_int(object, 0x438, 0x77777777);
    set_int(object, 0x444, 0x88888888);
    rotate_out_x[0] = 10;
    rotate_out_z[0] = 20;
    rotate_out_x[1] = 30;
    rotate_out_z[1] = 40;
}

int main(void)
{
    int ok;

    reset();
    body_point_place(object);
    ok = rotate_calls == 2
        && rotate_out_addr[0] == (int)(object + 0x434)
        && rotate_out_addr[1] == (int)(object + 0x440)
        && rotate_in_addr[0] == (int)(object + 0x41c)
        && rotate_in_addr[1] == (int)(object + 0x428)
        && rotate_angle[0] == 0xbfc00000
        && rotate_angle[1] == 0xbfc00000
        && get_int(object, 0x434) == 1010
        && get_int(object, 0x43c) == -1980
        && get_int(object, 0x440) == 1030
        && get_int(object, 0x448) == -1960
        && get_int(object, 0x438) == 0x77777777
        && get_int(object, 0x444) == 0x88888888
        && get_int(object, 0x41c) == 0x111
        && get_int(object, 0x424) == 0x112
        && get_int(object, 0x428) == 0x221
        && get_int(object, 0x430) == 0x222;
    put(ok ? '0' : 'x');

    reset();
    set_bits(object, 0x4a8, 0x00000000);
    set_int(object, 0x498, 100);
    set_int(object, 0x4a0, 200);
    rotate_out_x[0] = 5;
    rotate_out_z[0] = 6;
    rotate_out_x[1] = 7;
    rotate_out_z[1] = 8;
    body_point_place(object);
    ok = rotate_angle[0] == 0x80000000
        && rotate_angle[1] == 0x80000000
        && get_int(object, 0x434) == 105
        && get_int(object, 0x43c) == 206
        && get_int(object, 0x440) == 107
        && get_int(object, 0x448) == 208;
    put(ok ? '1' : 'x');

    reset();
    set_bits(object, 0x4a8, 0x80000000);
    set_int(object, 0x498, 0);
    set_int(object, 0x4a0, 0);
    rotate_out_x[0] = 5;
    rotate_out_z[0] = 6;
    rotate_out_x[1] = 7;
    rotate_out_z[1] = 8;
    body_point_place(object);
    ok = rotate_angle[0] == 0x00000000
        && rotate_angle[1] == 0x00000000
        && get_int(object, 0x434) == 5
        && get_int(object, 0x43c) == 6
        && get_int(object, 0x440) == 7
        && get_int(object, 0x448) == 8;
    put(ok ? '2' : 'x');

    reset();
    set_bits(object, 0x4a8, 0xc0000000);
    body_point_place(object);
    ok = rotate_angle[0] == 0x40000000
        && rotate_angle[1] == 0x40000000;
    put(ok ? '3' : 'x');

    reset();
    set_int(object, 0x498, 7);
    set_int(object, 0x4a0, 9);
    rotate_out_x[0] = 1;
    rotate_out_z[0] = 2;
    rotate_out_x[1] = 3;
    rotate_out_z[1] = 4;
    body_point_place(object);
    ok = get_int(object, 0x434) == 8
        && get_int(object, 0x43c) == 11
        && get_int(object, 0x440) == 10
        && get_int(object, 0x448) == 13
        && get_int(object, 0x41c) == 0x111
        && get_int(object, 0x428) == 0x221;
    put(ok ? '4' : 'x');

    reset();
    set_bits(object, 0x4a8, 0x40200000);
    body_point_place(object);
    ok = rotate_angle[0] == 0xc0200000
        && rotate_angle[1] == 0xc0200000;
    put(ok ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
