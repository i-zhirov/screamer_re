/* The test harness for the rebuilt `path_playback_update` module.
 *
 * The harness holds the playback block, the playback state, and ten keyframe
 * records.  The state points at the keyframes and the playback block points at
 * the state.  The `spline_interpolate` stub records its arguments and writes a
 * known position to the output.
 *
 * The harness runs nine paths:
 *
 *     0  the enable global is zero: the function writes nothing
 *     1  the state is not active: the function writes nothing
 *     2  the second argument is set: the function copies the first keyframe
 *        and does not interpolate
 *     3  the playback time stays below the next stamp: the function copies
 *        the head keyframe and interpolates with the fraction 0.5
 *     4  the playback time passes five stamps: the index lands on 5 and the
 *        function interpolates with the fraction 0.5
 *     5  the playback time passes the last stamp: the index reaches the count,
 *        the active flag becomes -1, and the function does not interpolate
 *     6  the head is one: the function does not interpolate
 *     7  the count minus the head is two: the function does not interpolate
 *     8  the index is zero and the time equals the first stamp: the function
 *        keeps the head at zero and does not interpolate
 *     D  the exit marker
 *
 * The float values arrive as bit patterns, because the minimal image does not
 * load the constant segment.
 */
#pragma off (check_stack)

void path_playback_update(char *out, int flag);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

float DAT_00118e10;
int DAT_0011d264;

static char text[8];
static char object[0x40];
static char state[0x30];
static char frames[0x28 * 10];

static int spline_calls;
static int *spline_points[4];
static int *spline_out;
static float spline_t;

static unsigned int float_bits(float value)
{
    union {
        float f;
        unsigned int u;
    } number;

    number.f = value;
    return number.u;
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

static int out_int(int offset)
{
    return *(int *)(object + offset);
}

static unsigned int out_bits(int offset)
{
    return *(unsigned int *)(object + offset);
}

static unsigned int frame_bits(int frame, int offset)
{
    return *(unsigned int *)(frames + frame * 0x28 + offset);
}

static void set_frame(int frame, int x, int y, int z, unsigned int a0,
    unsigned int a1, unsigned int a2, int f18, int f1c, unsigned int f20,
    float stamp)
{
    char *p = frames + frame * 0x28;

    *(int *)(p + 0x00) = x;
    *(int *)(p + 0x04) = y;
    *(int *)(p + 0x08) = z;
    *(unsigned int *)(p + 0x0c) = a0;
    *(unsigned int *)(p + 0x10) = a1;
    *(unsigned int *)(p + 0x14) = a2;
    *(int *)(p + 0x18) = f18;
    *(int *)(p + 0x1c) = f1c;
    *(unsigned int *)(p + 0x20) = f20;
    *(float *)(p + 0x24) = stamp;
}

static void reset(void)
{
    int index;

    for (index = 0; index < 0x40; index++) {
        object[index] = 0;
    }
    for (index = 0; index < 0x30; index++) {
        state[index] = 0;
    }
    for (index = 0; index < 0x28 * 10; index++) {
        frames[index] = 0;
    }
    for (index = 0; index < 10; index++) {
        set_frame(index, 0x100 + index, 0x200 + index, 0x300 + index,
            0x3f000000u + index, 0x40000000u + index, 0x40800000u + index,
            0x500 + index, 0x600 + index, 0x3f800000u + index, 0.0f);
    }
    spline_calls = 0;
    spline_out = 0;
    spline_t = 0.0f;
    for (index = 0; index < 4; index++) {
        spline_points[index] = 0;
    }
    *(char **)(object + 0x08) = state;
    *(int *)(state + 0x04) = 0;
    *(int *)(state + 0x08) = 10;
    *(int *)(state + 0x0c) = 1;
    *(char **)(state + 0x20) = frames;
    DAT_00118e10 = 0.0f;
    DAT_0011d264 = 1;
}

static void set_out_sentinel(void)
{
    int offset;

    for (offset = 0x10; offset <= 0x30; offset += 4) {
        *(unsigned int *)(object + offset) = 0xdeadbeef;
    }
}

static int sentinel(void)
{
    int offset;

    for (offset = 0x10; offset <= 0x30; offset += 4) {
        if (*(unsigned int *)(object + offset) != 0xdeadbeef) {
            return 0;
        }
    }
    return 1;
}

static int head_fields(int frame)
{
    return frame_bits(frame, 0x0c) == out_bits(0x1c)
        && frame_bits(frame, 0x10) == out_bits(0x20)
        && frame_bits(frame, 0x14) == out_bits(0x24)
        && frame_bits(frame, 0x18) == out_bits(0x28)
        && frame_bits(frame, 0x1c) == out_bits(0x2c)
        && frame_bits(frame, 0x20) == out_bits(0x30);
}

static int full_copy(int frame)
{
    return *(int *)(frames + frame * 0x28 + 0x00) == out_int(0x10)
        && *(int *)(frames + frame * 0x28 + 0x04) == out_int(0x14)
        && *(int *)(frames + frame * 0x28 + 0x08) == out_int(0x18)
        && head_fields(frame);
}

static int spline_position(void)
{
    return out_int(0x10) == 0x1001
        && out_int(0x14) == 0x1002
        && out_int(0x18) == 0x1003;
}

static int spline_points_are(int a, int b, int c, int d)
{
    return spline_points[0] == (int *)(frames + a * 0x28)
        && spline_points[1] == (int *)(frames + b * 0x28)
        && spline_points[2] == (int *)(frames + c * 0x28)
        && spline_points[3] == (int *)(frames + d * 0x28);
}

static void set_stamp(int frame, float stamp)
{
    *(float *)(frames + frame * 0x28 + 0x24) = stamp;
}

void spline_interpolate(int **points, int *out, float t)
{
    int index;

    spline_calls += 1;
    for (index = 0; index < 4; index++) {
        spline_points[index] = points[index];
    }
    spline_out = out;
    spline_t = t;
    out[0] = 0x1001;
    out[1] = 0x1002;
    out[2] = 0x1003;
}

int main(void)
{
    reset();
    DAT_0011d264 = 0;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(sentinel() && spline_calls == 0 ? '0' : 'x');

    reset();
    *(int *)(state + 0x0c) = 0;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(sentinel() && spline_calls == 0 ? '1' : 'x');

    reset();
    *(int *)(state + 0x04) = 5;
    set_out_sentinel();
    path_playback_update(object, 1);
    put(full_copy(0) && spline_calls == 0 ? '2' : 'x');

    reset();
    *(int *)(state + 0x04) = 3;
    set_stamp(2, 4.0f);
    set_stamp(3, 10.0f);
    DAT_00118e10 = 7.0f;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(*(int *)(state + 0x04) == 3
        && head_fields(2)
        && spline_calls == 1
        && spline_position()
        && spline_out == (int *)(object + 0x10)
        && float_bits(spline_t) == float_bits(0.5f)
        && spline_points_are(1, 2, 3, 4) ? '3' : 'x');

    reset();
    *(int *)(state + 0x04) = 0;
    *(int *)(state + 0x08) = 8;
    set_stamp(0, 0.0f);
    set_stamp(1, 5.0f);
    set_stamp(2, 10.0f);
    set_stamp(3, 15.0f);
    set_stamp(4, 20.0f);
    set_stamp(5, 30.0f);
    set_stamp(6, 40.0f);
    set_stamp(7, 50.0f);
    DAT_00118e10 = 25.0f;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(*(int *)(state + 0x04) == 5
        && head_fields(4)
        && spline_calls == 1
        && spline_position()
        && spline_out == (int *)(object + 0x10)
        && float_bits(spline_t) == float_bits(0.5f)
        && spline_points_are(3, 4, 5, 6) ? '4' : 'x');

    reset();
    *(int *)(state + 0x04) = 6;
    *(int *)(state + 0x08) = 8;
    set_stamp(6, 10.0f);
    set_stamp(7, 20.0f);
    set_stamp(8, 30.0f);
    DAT_00118e10 = 100.0f;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(*(int *)(state + 0x04) == 8
        && *(int *)(state + 0x0c) == -1
        && full_copy(7)
        && spline_calls == 0 ? '5' : 'x');

    reset();
    *(int *)(state + 0x04) = 2;
    set_stamp(2, 10.0f);
    DAT_00118e10 = 5.0f;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(*(int *)(state + 0x04) == 2
        && full_copy(1)
        && spline_calls == 0 ? '6' : 'x');

    reset();
    *(int *)(state + 0x04) = 3;
    *(int *)(state + 0x08) = 4;
    set_stamp(3, 10.0f);
    DAT_00118e10 = 5.0f;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(*(int *)(state + 0x04) == 3
        && full_copy(2)
        && spline_calls == 0 ? '7' : 'x');

    reset();
    *(int *)(state + 0x04) = 0;
    set_stamp(0, 10.0f);
    DAT_00118e10 = 10.0f;
    set_out_sentinel();
    path_playback_update(object, 0);
    put(*(int *)(state + 0x04) == 0
        && full_copy(0)
        && spline_calls == 0 ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
