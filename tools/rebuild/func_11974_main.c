/* The test harness for the rebuilt `func_11974` module.
 *
 * The harness holds a two-entry object list at the 0x1c-byte stride, the two
 * block objects, the effect-slot index, and the frame delta.  The object list
 * entry holds the block object at +0x0c and the file record at +0x14.  The
 * block object holds the frame counter at +0x20.
 *
 * The paths:
 *
 *     0  an active entry and the delta 1.0: the frame becomes 65535 and the
 *        result is 0
 *     1  a free entry: the frame stays at 7 and the result is 0x11
 *     2  the delta 0.5: the frame truncates to 32767 and the result is 0
 *     3  the second list entry and the delta 0.5: the second object frame
 *        becomes 32767 and the first object frame stays
 *     4  the delta -0.5: the frame truncates to -32767
 *     5  a frame of 1234 and the delta 0.0: the frame becomes 0
 *     6  the delta 2.0: the frame becomes 131070
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns.  The scale is 65535.0.
 */
#pragma off (check_stack)

int func_11974(void *table, int *slot, float delta);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct entry {
    char pad[0xc];
    char *object;
    int chunk;
    int flag;
    int slot;
};

struct object_block {
    char pad[0x20];
    int frame;
    char tail[0x40];
};

static char text[8];
static struct entry list[2];
static struct object_block objects[2];
static int slot_index;
float func_11974_scale;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
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

static void check(int index, int ok)
{
    put(ok ? (char)('0' + index) : 'x');
}

static void reset(void)
{
    int index;

    for (index = 0; index < 2; index++) {
        list[index].object = (char *)&objects[index];
        list[index].chunk = 0;
        list[index].flag = 1;
        list[index].slot = index;
        objects[index].frame = 0;
    }
    slot_index = 0;
}

int main(void)
{
    float delta;
    int result;

    set_bits_float(&func_11974_scale, 0x477fff00);

    reset();
    set_bits_float(&delta, 0x3f800000);
    result = func_11974(list, &slot_index, delta);
    check(0, result == 0 && objects[0].frame == 65535);

    reset();
    list[0].flag = 0;
    objects[0].frame = 7;
    set_bits_float(&delta, 0x3f800000);
    result = func_11974(list, &slot_index, delta);
    check(1, result == 0x11 && objects[0].frame == 7);

    reset();
    set_bits_float(&delta, 0x3f000000);
    result = func_11974(list, &slot_index, delta);
    check(2, result == 0 && objects[0].frame == 32767);

    reset();
    slot_index = 1;
    objects[1].frame = 5;
    set_bits_float(&delta, 0x3f000000);
    result = func_11974(list, &slot_index, delta);
    check(3, result == 0 && objects[1].frame == 32767 && objects[0].frame == 0);

    reset();
    set_bits_float(&delta, 0xbf000000);
    result = func_11974(list, &slot_index, delta);
    check(4, result == 0 && objects[0].frame == -32767);

    reset();
    objects[0].frame = 1234;
    set_bits_float(&delta, 0x00000000);
    result = func_11974(list, &slot_index, delta);
    check(5, result == 0 && objects[0].frame == 0);

    reset();
    set_bits_float(&delta, 0x40000000);
    result = func_11974(list, &slot_index, delta);
    check(6, result == 0 && objects[0].frame == 131070);

    put('D');
    quit();
    return 0;
}
