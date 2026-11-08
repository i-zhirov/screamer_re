/* The test harness for the rebuilt `sound_source_frame_select` module.
 *
 * The harness holds the sound context, the voice record with the camera matrix,
 * the object with the three shorts and the identifier, the three-band frame
 * table, the distance limits, the per-index distance table, and the frame base.
 *
 * The harness runs ten paths:
 *
 *     0  a far point selects the 0x208 band and stores the frame and distance
 *     1  a middle point selects the 0x104 band and the `a` name skips the bias
 *     2  a near point selects the 0 band and the `A` name skips the bias
 *     3  the `i` name biases the distance by -4000
 *     4  the `x` name biases the distance by -1000
 *     5  a zero second name byte with a set second byte skips the bias
 *     6  a zero frame offset gives the null frame
 *     7  a scaled and translated matrix forms the distance 115
 *     8  the square root of 8 rounds to the nearest integer 3
 *     9  a zero point stores the null distance in the third index
 *     D  the exit marker
 *
 * The distance table is checked through the object, and the frame pointer is
 * checked in the voice record and in the object field.
 */
#pragma off (check_stack)

void sound_source_frame_select(char *context);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define CONTEXT_INDEX 0x08
#define CONTEXT_VOICE 0x0c
#define CONTEXT_OBJECT 0x14
#define CONTEXT_TABLE 0xb4
#define CONTEXT_BASE 0xb8

#define OBJECT_POINT 0x04
#define OBJECT_NAME 0x68

#define VOICE_FRAME 0x00
#define VOICE_DISTANCE 0x10
#define VOICE_MATRIX 0x118

#define BAND_MIDDLE_INDEX 0x41
#define BAND_FAR_INDEX 0x82

struct source_distance {
    short id;
    short distance;
};

int distance_limit_near;
int distance_limit_far;
struct source_distance source_distances[8];

static char text[8];
static char context[0x100];
static char voice[0x180];
static char object[0x100];
static int frame_table[0x84];
static char frame_base[0x180];

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

static void set_short(char *base, int offset, short value)
{
    *(short *)(base + offset) = value;
}

static short get_short(char *base, int offset)
{
    return *(short *)(base + offset);
}

static void set_ptr(char *base, int offset, char *value)
{
    *(char **)(base + offset) = value;
}

static char *get_ptr(char *base, int offset)
{
    return *(char **)(base + offset);
}

static void set_point(int x, int y, int z)
{
    set_short(object, OBJECT_POINT + 0, (short)x);
    set_short(object, OBJECT_POINT + 2, (short)y);
    set_short(object, OBJECT_POINT + 4, (short)z);
}

static void reset(void)
{
    int index;

    clear_bytes(context, 0x100);
    clear_bytes(voice, 0x180);
    clear_bytes(object, 0x100);
    clear_bytes((char *)frame_table, sizeof(frame_table));
    clear_bytes(frame_base, 0x180);
    distance_limit_near = -500;
    distance_limit_far = -1500;
    for (index = 0; index < 8; index++) {
        source_distances[index].id = (short)(0x100 + index);
        source_distances[index].distance = 0x7777;
    }
    set_ptr(context, CONTEXT_VOICE, voice);
    set_ptr(context, CONTEXT_OBJECT, object);
    set_ptr(context, CONTEXT_TABLE, (char *)frame_table);
    set_int(context, CONTEXT_BASE, (int)frame_base);
    set_int(context, CONTEXT_INDEX, 0);
    set_int(voice, VOICE_MATRIX + 0x00, 1 << 14);
    set_int(voice, VOICE_MATRIX + 0x10, 1 << 14);
    set_int(voice, VOICE_MATRIX + 0x20, 1 << 14);
    frame_table[0] = 0x10;
    frame_table[BAND_MIDDLE_INDEX] = 0x80;
    frame_table[BAND_FAR_INDEX] = 0xc0;
}

int main(void)
{
    int ok;

    reset();
    set_point(2000, 0, 0);
    set_int(object, OBJECT_NAME, 0x41);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -2000
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0xc0
        && get_ptr(context, CONTEXT_OBJECT) == frame_base + 0xc0
        && get_short((char *)source_distances, 2) == -2000
        && get_short((char *)source_distances, 0) == 0x100;
    put(ok ? '0' : 'x');

    reset();
    set_point(600, 0, 0);
    set_int(object, OBJECT_NAME, 0x61);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -600
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x80
        && get_ptr(context, CONTEXT_OBJECT) == frame_base + 0x80
        && get_short((char *)source_distances, 2) == -600;
    put(ok ? '1' : 'x');

    reset();
    set_point(100, 0, 0);
    set_int(object, OBJECT_NAME, 0x41);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -100
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_ptr(context, CONTEXT_OBJECT) == frame_base + 0x10
        && get_short((char *)source_distances, 2) == -100;
    put(ok ? '2' : 'x');

    reset();
    set_point(100, 0, 0);
    set_int(object, OBJECT_NAME, 0x00690046);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -4100
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_short((char *)source_distances, 2) == -4100;
    put(ok ? '3' : 'x');

    reset();
    set_point(100, 0, 0);
    set_int(object, OBJECT_NAME, 0x00780046);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -1100
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_short((char *)source_distances, 2) == -1100;
    put(ok ? '4' : 'x');

    reset();
    set_point(100, 0, 0);
    set_int(object, OBJECT_NAME, 0x00697a46);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -100
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_short((char *)source_distances, 2) == -100;
    put(ok ? '5' : 'x');

    reset();
    set_point(100, 0, 0);
    set_int(object, OBJECT_NAME, 0x41);
    frame_table[0] = 0;
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -100
        && get_ptr(voice, VOICE_FRAME) == 0
        && get_ptr(context, CONTEXT_OBJECT) == 0
        && get_short((char *)source_distances, 2) == -100;
    put(ok ? '6' : 'x');

    reset();
    set_point(3, 4, -2);
    set_int(object, OBJECT_NAME, 0x41);
    set_int(voice, VOICE_MATRIX + 0x00, 2 << 14);
    set_int(voice, VOICE_MATRIX + 0x10, 3 << 14);
    set_int(voice, VOICE_MATRIX + 0x20, 1 << 14);
    set_int(voice, VOICE_MATRIX + 0x24, 100);
    set_int(voice, VOICE_MATRIX + 0x28, -50);
    set_int(voice, VOICE_MATRIX + 0x2c, 25);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -115
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_short((char *)source_distances, 2) == -115;
    put(ok ? '7' : 'x');

    reset();
    set_point(2, 2, 0);
    set_int(object, OBJECT_NAME, 0x41);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == -3
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_short((char *)source_distances, 2) == -3;
    put(ok ? '8' : 'x');

    reset();
    set_point(0, 0, 0);
    set_int(object, OBJECT_NAME, 0x41);
    set_int(context, CONTEXT_INDEX, 3);
    sound_source_frame_select(context);
    ok = get_int(voice, VOICE_DISTANCE) == 0
        && get_ptr(voice, VOICE_FRAME) == frame_base + 0x10
        && get_short((char *)source_distances + 3 * 4, 2) == 0
        && get_short((char *)source_distances + 3 * 4, 0) == 0x103;
    put(ok ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
