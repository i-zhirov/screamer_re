/* The test harness for the rebuilt `player_view_orbit` module.
 *
 * The harness holds the player object, the view object, the four constants,
 * and the three callee stubs.  The stubs count their calls and save the
 * arguments.  The `object_value_approach` and `object_angle_approach` stubs
 * save the target, the value, and the index, but they do not change the
 * target, so the final coordinates use the angles the test sets.
 *
 * The harness runs eight paths:
 *
 *     0  the view mode 0 places the view at (car x, car y + 300, car z + 1700)
 *     1  the view mode 1 uses the radius 1300 and the height offset 300
 *     2  the view mode 2 uses the radius 1400 and the height offset 100
 *     3  the pitch -pi/2 lifts the height by 1700 and keeps the z coordinate
 *     4  the yaw above 2 pi wraps down by 2 pi
 *     5  the yaw below zero wraps up by 2 pi
 *     6  the clear flag approaches the height and the three angles through the
 *        stubs with the index 0x51 and keeps the height cell
 *     7  the view mode 1 takes the long pitch offset
 *     D  the exit marker
 *
 * The float constants and the expected integers come from the same arithmetic
 * as the original.  The pitch case exercises the sine near its maximum, so the
 * height truncates one unit below the exact sum.
 */
#pragma off (check_stack)

void player_view_orbit(char *car, char *view, int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float player_view_two_pi;
float player_view_minus_two_pi;
double player_view_offset_short;
double player_view_offset_long;

static char text[8];
static char car[0x500];
static char view[0x80];

static int copy_calls;
static void *copy_table[2];
static char *copy_name[2];
static int *copy_slot[2];

static int value_calls;
static int *value_target;
static float value_target_value;
static int value_index;

static int angle_calls;
static float *angle_target[3];
static float angle_value[3];
static int angle_index[3];

void object_list_copy(void *table, char *name, int *slot)
{
    if (copy_calls < 2) {
        copy_table[copy_calls] = table;
        copy_name[copy_calls] = name;
        copy_slot[copy_calls] = slot;
    }
    copy_calls = copy_calls + 1;
}

void object_value_approach(int *target, float value, int index)
{
    value_calls = value_calls + 1;
    value_target = target;
    value_target_value = value;
    value_index = index;
}

void object_angle_approach(float *target, float value, int index)
{
    if (angle_calls < 3) {
        angle_target[angle_calls] = target;
        angle_value[angle_calls] = value;
        angle_index[angle_calls] = index;
    }
    angle_calls = angle_calls + 1;
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

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static void reset_case(void)
{
    clear_bytes(car, 0x500);
    clear_bytes(view, 0x80);

    copy_calls = 0;
    copy_table[0] = 0;
    copy_table[1] = 0;
    copy_name[0] = 0;
    copy_name[1] = 0;
    copy_slot[0] = 0;
    copy_slot[1] = 0;

    value_calls = 0;
    value_target = 0;
    value_target_value = 0.0f;
    value_index = -1;

    angle_calls = 0;
    angle_target[0] = 0;
    angle_target[1] = 0;
    angle_target[2] = 0;
    angle_value[0] = 0.0f;
    angle_value[1] = 0.0f;
    angle_value[2] = 0.0f;
    angle_index[0] = -1;
    angle_index[1] = -1;
    angle_index[2] = -1;

    set_float(&player_view_two_pi, 0x40c90fdb);
    set_float(&player_view_minus_two_pi, 0xc0c90fdb);
    player_view_offset_short = 0.0;
    player_view_offset_long = 0.0;
}

static void set_car_int(int offset, int value)
{
    *(int *)(car + offset) = value;
}

static int copy_is_ready(void)
{
    return copy_calls == 2
        && copy_table[0] == (void *)0x6530
        && copy_name[0] == (char *)0x6350
        && copy_slot[0] == (int *)(car + 0x460)
        && copy_table[1] == (void *)0x6530
        && copy_name[1] == (char *)0x6350
        && copy_slot[1] == (int *)(car + 0x468);
}

static int view_is(int x, int y, int z)
{
    return *(int *)(view + 0x00) == x
        && *(int *)(view + 0x04) == y
        && *(int *)(view + 0x08) == z;
}

static int view_prev_is(int x, int z)
{
    return *(int *)(view + 0x0c) == x
        && *(int *)(view + 0x14) == z;
}

int main(void)
{
    reset_case();
    set_car_int(0x4b0, 1000);
    set_car_int(0x4b4, 2000);
    set_car_int(0x4b8, 3000);
    set_float((float *)(car + 0x4d0), 0x3f000000);
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 0);
    put(view_is(1000, 2300, 4700) && view_prev_is(1000, 4700)
        && *(float *)(view + 0x30) == 0.0f
        && *(float *)(view + 0x34) == 0.0f
        && *(float *)(view + 0x38) == 0.5f
        && copy_is_ready() && angle_calls == 0 && value_calls == 0
        ? '0' : 'x');

    reset_case();
    set_car_int(0x4b0, 2000);
    set_car_int(0x4b4, 1000);
    set_car_int(0x4b8, 4000);
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 1);
    put(view_is(2000, 1300, 5300) && view_prev_is(2000, 5300)
        && *(float *)(view + 0x30) == 0.0f && copy_is_ready()
        ? '1' : 'x');

    reset_case();
    set_car_int(0x4b0, 500);
    set_car_int(0x4b4, 600);
    set_car_int(0x4b8, 700);
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 2);
    put(view_is(500, 700, 2100) && view_prev_is(500, 2100)
        && *(float *)(view + 0x30) == 0.0f && copy_is_ready()
        ? '2' : 'x');

    reset_case();
    set_car_int(0x4b0, 100);
    set_car_int(0x4b4, 200);
    set_car_int(0x4b8, 300);
    set_float((float *)(car + 0x4d4), 0xbfc90fdb);
    set_float((float *)(view + 0x30), 0xbfc90fdb);
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 0);
    put(view_is(100, 2199, 299) && view_prev_is(100, 299)
        && *(float *)(view + 0x30) == -1.5707964f && copy_is_ready()
        ? '3' : 'x');

    reset_case();
    set_car_int(0x4b0, 0);
    set_car_int(0x4b4, 0);
    set_car_int(0x4b8, 0);
    set_float((float *)(car + 0x4cc), 0x40e00000);
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 0);
    put(*(int *)(view + 0x04) == 300
        && *(float *)(car + 0x4cc) == 0.7168145f
        && *(float *)(view + 0x34) == *(float *)(car + 0x4cc)
        && copy_is_ready()
        ? '4' : 'x');

    reset_case();
    set_car_int(0x4b0, 0);
    set_car_int(0x4b4, 0);
    set_car_int(0x4b8, 0);
    set_float((float *)(car + 0x4cc), 0xbf800000);
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 0);
    put(*(int *)(view + 0x04) == 300
        && *(float *)(car + 0x4cc) == 5.2831855f
        && *(float *)(view + 0x34) == *(float *)(car + 0x4cc)
        && copy_is_ready()
        ? '5' : 'x');

    reset_case();
    set_car_int(0x4b0, 1000);
    set_car_int(0x4b4, 2000);
    set_car_int(0x4b8, 3000);
    set_float((float *)(car + 0x4d0), 0x3f000000);
    set_float((float *)(car + 0x4cc), 0x3f19999a);
    set_float((float *)(car + 0x4dc), 0x3f333333);
    set_float((float *)(car + 0x4d4), 0x3f000000);
    player_view_offset_short = 0.25;
    *(int *)(view + 4) = 0x1234;
    player_view_orbit(car, view, 0);
    put(view_is(1000, 0x1234, 4700) && view_prev_is(1000, 4700)
        && *(int *)(view + 0x04) == 0x1234
        && value_calls == 1 && value_target == (int *)(view + 4)
        && value_target_value == 2300.0f && value_index == 0
        && angle_calls == 3 && angle_index[0] == 0x51
        && angle_index[1] == 0x51 && angle_index[2] == 0x51
        && angle_target[0] == (float *)(view + 0x34)
        && angle_target[1] == (float *)(view + 0x38)
        && angle_target[2] == (float *)(view + 0x30)
        && angle_value[0] == 0.6f && angle_value[1] == 0.7f
        && angle_value[2] == 0.75f
        ? '6' : 'x');

    reset_case();
    set_car_int(0x4b0, 100);
    set_car_int(0x4b4, 200);
    set_car_int(0x4b8, 300);
    set_float((float *)(car + 0x4d4), 0x3f800000);
    player_view_offset_long = 0.25;
    *(int *)(view + 0x7c) = 1;
    player_view_orbit(car, view, 1);
    put(*(int *)(view + 0x04) == 500
        && *(float *)(view + 0x30) == 1.25f && copy_is_ready()
        ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
