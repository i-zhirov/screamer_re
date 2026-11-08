/* The test harness for the rebuilt `traffic_push_response` module.
 *
 * The harness holds one 0x900-byte player object and the `table_lookup` stub.
 * The stub records the call count and the index.
 *
 * The harness runs six paths:
 *
 *     0  the flag is zero: the function looks up the entry 1, sets the turn
 *        rate to 5000.0, and returns before the contact block
 *     1  the flag is set and the distance is zero: the function sets the
 *        distance to 1, forms the terms 4.0 and 6.0, and moves the player
 *        position from 100 to 136 and 154
 *     2  the distance is 8: the terms are 0.5 and 0.75, and the player
 *        position takes 101 and the truncated 101 from the 101.5 sum
 *     3  the negative deltas give a negative sum: the truncation goes toward
 *        zero, so the positions take -96 and -94
 *     4  the negative distance gives the terms -2.0 and -3.0: the player
 *        position takes -24 and -36
 *     5  the flag is 2: any non-zero flag opens the contact block, so the
 *        result equals path 1
 *     D  the exit marker
 *
 * The float checks compare the bit patterns exactly, because a float literal
 * in this minimal image reads the constant segment, which this harness does
 * not load.
 */
#pragma off (check_stack)

void traffic_push_response(char *self, int push);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char self_object[0x900];
static char text[8];

static int lookup_calls;
static int lookup_arg;

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_arg = index;
    return 0;
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

static void set_int(char *object, int offset, int value)
{
    *(int *)(object + offset) = value;
}

static int int_is(char *object, int offset, int value)
{
    return *(int *)(object + offset) == value;
}

static unsigned int float_bits(char *object, int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(object + offset);
    return value.u;
}

static int float_is(char *object, int offset, unsigned int bits)
{
    return float_bits(object, offset) == bits;
}

static void reset(void)
{
    clear_bytes(self_object, 0x900);
    lookup_calls = 0;
    lookup_arg = 0;
}

int main(void)
{
    reset();
    set_int(self_object, 0x55c, 0);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(self_object, 0x4b0, 100);
    set_int(self_object, 0x4b8, 100);
    traffic_push_response(self_object, 0);
    put(lookup_calls == 1 && lookup_arg == 1
        && float_is(self_object, 0x54c, 0x459c4000)
        && int_is(self_object, 0x55c, 0)
        && int_is(self_object, 0x4b0, 100)
        && int_is(self_object, 0x4b8, 100)
        && float_is(self_object, 0x550, 0)
        && float_is(self_object, 0x558, 0)
        && float_is(self_object, 0x80c, 0)
        && float_is(self_object, 0x814, 0) ? '0' : 'x');

    reset();
    set_int(self_object, 0x55c, 0);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(self_object, 0x4b0, 100);
    set_int(self_object, 0x4b8, 100);
    traffic_push_response(self_object, 1);
    put(lookup_calls == 1 && lookup_arg == 1
        && int_is(self_object, 0x55c, 1)
        && float_is(self_object, 0x550, 0x40800000)
        && float_is(self_object, 0x558, 0x40c00000)
        && float_is(self_object, 0x80c, 0x40800000)
        && float_is(self_object, 0x814, 0x40c00000)
        && int_is(self_object, 0x4b0, 136)
        && int_is(self_object, 0x4b8, 154) ? '1' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(self_object, 0x4b0, 100);
    set_int(self_object, 0x4b8, 100);
    traffic_push_response(self_object, 1);
    put(float_is(self_object, 0x550, 0x3f000000)
        && float_is(self_object, 0x558, 0x3f400000)
        && float_is(self_object, 0x80c, 0x3f000000)
        && float_is(self_object, 0x814, 0x3f400000)
        && int_is(self_object, 0x4b0, 101)
        && int_is(self_object, 0x4b8, 101) ? '2' : 'x');

    reset();
    set_int(self_object, 0x55c, 8);
    set_int(self_object, 0x560, 1);
    set_int(self_object, 0x564, -4);
    set_int(self_object, 0x56c, -6);
    set_int(self_object, 0x4b0, -100);
    set_int(self_object, 0x4b8, -100);
    traffic_push_response(self_object, 1);
    put(float_is(self_object, 0x550, 0xbf000000)
        && float_is(self_object, 0x558, 0xbf400000)
        && int_is(self_object, 0x4b0, -96)
        && int_is(self_object, 0x4b8, -94) ? '3' : 'x');

    reset();
    set_int(self_object, 0x55c, -2);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(self_object, 0x4b0, 0);
    set_int(self_object, 0x4b8, 0);
    traffic_push_response(self_object, 1);
    put(float_is(self_object, 0x550, 0xc0000000)
        && float_is(self_object, 0x558, 0xc0400000)
        && int_is(self_object, 0x4b0, -24)
        && int_is(self_object, 0x4b8, -36) ? '4' : 'x');

    reset();
    set_int(self_object, 0x55c, 0);
    set_int(self_object, 0x560, 10);
    set_int(self_object, 0x564, 4);
    set_int(self_object, 0x56c, 6);
    set_int(self_object, 0x4b0, 100);
    set_int(self_object, 0x4b8, 100);
    traffic_push_response(self_object, 2);
    put(lookup_calls == 1 && lookup_arg == 1
        && int_is(self_object, 0x55c, 1)
        && float_is(self_object, 0x550, 0x40800000)
        && float_is(self_object, 0x558, 0x40c00000)
        && int_is(self_object, 0x4b0, 136)
        && int_is(self_object, 0x4b8, 154) ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
