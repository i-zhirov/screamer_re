/* The test harness for the rebuilt `player_car_link` module.
 *
 * The harness holds the player object, the four entry pairs of the 28-byte
 * table at `DAT_0010653c`, the render arrays, the constants, and the screen
 * projection stub that stands in for the function at 0x18ccc.
 *
 * The harness runs eight paths:
 *
 *     0  the full path: the screen positions, the three masked angles, the two
 *        render values, the first masked pitch, the speed difference, the
 *        target copy, and the two link fields
 *     1  the state at `+0x148` is 1: the target keeps its two angle fields and
 *        the negative speed difference masks with 0x7fe
 *     2  the object field `+0x468` selects the second target
 *     3  the divisor at `+0x10c` is 2.0: the first masked pitch changes
 *     4  the speed 270.0 with the stored value 9 clears the low bit
 *     5  the angle sum 2 pi masks to 0 through the low 11 bits
 *     6  the world position on the second axis: the screen offsets follow
 *     7  the object field `+0x460` selects the second link and render pair
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because the minimal image
 * does not load the constant segment.
 */
#pragma off (check_stack)

void player_car_link(char *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

extern double sqrt(double value);

#define OBJECT_SIZE 0x800
#define LINK_SIZE 0x100
#define RENDER_SIZE 0x300
#define TARGET_SIZE 0x100

static char text[8];
static unsigned char object[OBJECT_SIZE];
static unsigned char link0[LINK_SIZE];
static unsigned char link1[LINK_SIZE];
static unsigned char render0[RENDER_SIZE];
static unsigned char render1[RENDER_SIZE];
static unsigned char target0[TARGET_SIZE];
static unsigned char target1[TARGET_SIZE];

unsigned char DAT_0010653c[0x100];
int DAT_00110a20;
int DAT_00110a28;
float player_car_link_angle_scale;
float player_car_link_pi;
float player_car_link_angle_double;
float player_car_link_angle_quad;
double player_car_link_pitch_ratio;
double player_car_link_pi_double;
float player_car_link_speed_divisor;
float player_car_link_screen_limit;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(unsigned char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void set_bits_double(double *p, unsigned int low, unsigned int high)
{
    union {
        double d;
        unsigned int u[2];
    } value;

    value.u[0] = low;
    value.u[1] = high;
    *p = value.d;
}

static void set_object_int(int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int object_int(int offset)
{
    return *(int *)(object + offset);
}

static void set_object_float(int offset, unsigned int bits)
{
    set_bits_float((float *)(object + offset), bits);
}

static void set_object_float_value(int offset, float value)
{
    *(float *)(object + offset) = value;
}

static void set_entry(int index, unsigned char *link, unsigned char *render)
{
    *(unsigned char **)(DAT_0010653c + index * 28) = link;
    *(unsigned char **)(DAT_0010653c + index * 28 + 4) = render;
}

static int link_int(unsigned char *link, int offset)
{
    return *(int *)(link + offset);
}

static int render_int(unsigned char *render, int index, int offset)
{
    return *(int *)(render + index * 32 + offset);
}

static int target_int(unsigned char *target, int offset)
{
    return *(int *)(target + offset);
}

void player_screen_offset(char *record)
{
    int dx;
    int dz;
    int distance;
    float length;
    float ratio_x;
    float ratio_z;

    dx = DAT_00110a20 - *(int *)(record + 0x4b0);
    dz = DAT_00110a28 - *(int *)(record + 0x4b8);
    distance = dx * dx + dz * dz;
    length = (float)sqrt((double)distance);
    ratio_x = (float)(DAT_00110a20 - *(int *)(record + 0x4b0)) / length;
    ratio_z = (float)(DAT_00110a28 - *(int *)(record + 0x4b8)) / length;
    *(int *)(record + 0x118) = (int)((float)*(int *)(record + 0x4b0)
        - ratio_x * player_car_link_screen_limit);
    *(int *)(record + 0x11c) = *(int *)(record + 0x4b4) - 20;
    *(int *)(record + 0x120) = (int)((float)*(int *)(record + 0x4b8)
        - ratio_z * player_car_link_screen_limit);
}

static void reset(void)
{
    clear_bytes(object, OBJECT_SIZE);
    clear_bytes(link0, LINK_SIZE);
    clear_bytes(link1, LINK_SIZE);
    clear_bytes(render0, RENDER_SIZE);
    clear_bytes(render1, RENDER_SIZE);
    clear_bytes(target0, TARGET_SIZE);
    clear_bytes(target1, TARGET_SIZE);
    clear_bytes(DAT_0010653c, 0x100);

    set_entry(0, link0, render0);
    set_entry(1, link1, render1);
    set_entry(2, target0, render0);
    set_entry(3, target1, render1);

    set_bits_float(&player_car_link_angle_scale, 0x44800000);
    set_bits_float(&player_car_link_pi, 0x40490fdb);
    set_bits_float(&player_car_link_angle_double, 0x40000000);
    set_bits_float(&player_car_link_angle_quad, 0x40800000);
    set_bits_double(&player_car_link_pitch_ratio, 0x00000000, 0x3ff80000);
    set_bits_double(&player_car_link_pi_double, 0x5fffffff, 0x400921fb);
    set_bits_float(&player_car_link_speed_divisor, 0x41700000);
    set_bits_float(&player_car_link_screen_limit, 0x41a00000);

    DAT_00110a20 = 60;
    DAT_00110a28 = 0;

    set_object_int(0x460, 0);
    set_object_int(0x468, 2);
    set_object_int(0x130, 1);
    set_object_int(0x134, 2);
    set_object_int(0x138, 3);
    set_object_int(0x4b0, 0);
    set_object_int(0x4b4, 50);
    set_object_int(0x4b8, 0);
    set_object_float(0x4c8, 0x40490fdb);
    set_object_float(0x4cc, 0xc0490fdb);
    set_object_float(0x4d0, 0x00000000);
    set_object_float(0x124, 0x00000000);
    set_object_float(0x128, 0x00000000);
    set_object_float(0x12c, 0x00000000);
    set_object_float(0x4e0, 0x3fc90fdb);
    set_object_float(0x4e8, 0x3ec90fdb);
    set_object_int(0x45c, 1);
    set_object_int(0x454, 2);
    set_object_int(0x44c, 3);
    set_object_int(0x464, 4);
    set_object_int(0x28c, 5);
    set_object_int(0x284, 6);
    set_object_int(0x1e4, 7);
    set_object_int(0x1dc, 8);
    set_object_int(0x230, 9);
    set_object_int(0x2d8, 10);
    set_object_float(0x108, 0x3f800000);
    set_object_float(0x10c, 0x3f800000);
    set_object_float(0x50c, 0x43160000);
    set_object_int(0x4c0, 2);
    set_object_int(0x148, 0);
    set_object_int(0x4ec, 42);
    set_object_int(0x7b0, 0x1234);
    set_object_int(0x7b4, 0x5678);

    *(int *)(render0 + 8 * 32 + 8) = 3;
}

static int full_path(void)
{
    return link_int(link0, 8) == 1
        && link_int(link0, 0xc) == 52
        && link_int(link0, 0x10) == 3
        && link_int(link0, 0x14) == 1024
        && link_int(link0, 0x18) == 1024
        && link_int(link0, 0x1c) == 0
        && link_int(link0, 0x3c) == 0x1234
        && link_int(link0, 0x20) == 0x5678
        && target_int(target0, 0x18) == 1024
        && target_int(target0, 8) == -20
        && target_int(target0, 0x10) == 0
        && target_int(target0, 0xc) == 40
        && target_int(target0, 0x14) == 1024
        && target_int(target0, 0x1c) == 0
        && render_int(render0, 1, 8) == 1024
        && render_int(render0, 2, 8) == 1024
        && render_int(render0, 3, 8) == 1024
        && render_int(render0, 4, 8) == 1024
        && render_int(render0, 1, 0x10) == 512
        && render_int(render0, 2, 0x10) == 512
        && render_int(render0, 3, 0x10) == 512
        && render_int(render0, 4, 0x10) == 512
        && render_int(render0, 1, 0x18) == -2
        && render_int(render0, 2, 0x18) == -2
        && render_int(render0, 3, 0x18) == -2
        && render_int(render0, 4, 0x18) == -2
        && render_int(render0, 5, 0xc) == 217
        && render_int(render0, 6, 0xc) == 217
        && render_int(render0, 7, 0xc) == 217
        && render_int(render0, 8, 0xc) == 217
        && render_int(render0, 8, 8) == 6
        && render_int(render0, 6, 8) == 6
        && render_int(render0, 9, 8) == 6
        && render_int(render0, 10, 8) == 6
        && link_int(link1, 8) == 0
        && target_int(target1, 8) == 0;
}

int main(void)
{
    reset();
    player_car_link(object);
    put(full_path() ? '0' : 'x');

    reset();
    set_object_int(0x148, 1);
    set_object_float(0x50c, 0x41700000);
    *(int *)(render0 + 8 * 32 + 8) = 20;
    *(int *)(target0 + 0x14) = 111;
    *(int *)(target0 + 0x1c) = 222;
    player_car_link(object);
    put(target_int(target0, 0x14) == 111
        && target_int(target0, 0x1c) == 222
        && render_int(render0, 8, 8) == 2028
        && render_int(render0, 6, 8) == 2028
        && render_int(render0, 9, 8) == 2028
        && render_int(render0, 10, 8) == 2028
        && target_int(target0, 0xc) == 40 ? '1' : 'x');

    reset();
    set_object_int(0x468, 3);
    player_car_link(object);
    put(target_int(target1, 0x18) == 1024
        && target_int(target1, 8) == -20
        && target_int(target1, 0x10) == 0
        && target_int(target1, 0xc) == 40
        && target_int(target1, 0x14) == 1024
        && target_int(target1, 0x1c) == 0
        && target_int(target0, 8) == 0
        && target_int(target0, 0xc) == 0 ? '2' : 'x');

    reset();
    set_object_float(0x10c, 0x40000000);
    player_car_link(object);
    put(render_int(render0, 5, 0xc) == 108
        && render_int(render0, 6, 0xc) == 108
        && render_int(render0, 7, 0xc) == 108
        && render_int(render0, 8, 0xc) == 108 ? '3' : 'x');

    reset();
    set_object_float(0x50c, 0x43870000);
    *(int *)(render0 + 8 * 32 + 8) = 9;
    player_car_link(object);
    put(render_int(render0, 8, 8) == 8
        && render_int(render0, 6, 8) == 8
        && render_int(render0, 9, 8) == 8
        && render_int(render0, 10, 8) == 8 ? '4' : 'x');

    reset();
    set_object_float(0x124, 0x40490fdb);
    player_car_link(object);
    put(link_int(link0, 0x14) == 0
        && link_int(link0, 0x18) == 1024 ? '5' : 'x');

    reset();
    DAT_00110a20 = 0;
    DAT_00110a28 = 80;
    player_car_link(object);
    put(target_int(target0, 8) == 0
        && target_int(target0, 0x10) == -20
        && link_int(link0, 8) == 1
        && link_int(link0, 0x10) == 3 ? '6' : 'x');

    reset();
    set_object_int(0x460, 1);
    set_object_int(0x468, 3);
    player_car_link(object);
    put(link_int(link1, 8) == 1
        && link_int(link1, 0xc) == 52
        && link_int(link1, 0x10) == 3
        && link_int(link1, 0x14) == 1024
        && link_int(link1, 0x3c) == 0x1234
        && link_int(link1, 0x20) == 0x5678
        && render_int(render1, 1, 8) == 1024
        && render_int(render1, 8, 8) == 10
        && target_int(target1, 0x18) == 1024
        && link_int(link0, 8) == 0
        && render_int(render0, 1, 8) == 0
        && target_int(target0, 8) == 0 ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
