/* The test harness for the rebuilt `sla_marker_check` module.
 *
 * The harness holds the player object, four 0x20-byte SLA markers, the marker
 * count, and one `table_lookup` stub.  The stub records the call count and the
 * index.
 *
 * The harness runs these paths:
 *
 *     0  the marker count is zero: no marker is tested
 *     1  one marker in range with a clear flag: the flag sets, the hit count
 *        rises, the score takes the value, and the lookup runs with 9
 *     2  the marker flag is already set: the function returns and no hit
 *        counts
 *     3  the first marker is outside the x radius: the counter advances and
 *        the second marker sets
 *     4  the first marker is outside the y radius: the counter advances and
 *        the second marker sets
 *     5  the square root stays above the radius: the marker is skipped and the
 *        second marker sets
 *     6  the square root equals the radius: the marker stays in range
 *     7  a skipped marker advances the counter and a negative value lowers the
 *        score from 10 to 7
 *     8  a skipped marker reaches an already set flag: the function returns
 *        with no hit
 *     9  the absolute x delta equals the radius: the marker is skipped
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because a float literal in
 * this minimal image reads the constant segment, which this harness does not
 * load.
 */
#pragma off (check_stack)

void sla_marker_check(void *object);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define STRIDE 0x20
#define MARKERS 4

int sla_marker_count;

struct sla_marker {
    int radius;
    int reserved04;
    int reserved08;
    int reserved0c;
    int value;
    int x;
    int reserved18;
    int y;
};

struct sla_marker sla_marker_table[MARKERS];

static char self_object[0x900];
static char text[8];

static int lookup_calls;
static int lookup_index;

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_index = index;
    return index;
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

static void set_float_bits(char *object, int offset, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *(float *)(object + offset) = value.f;
}

static unsigned int float_bits_at(char *object, int offset)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.f = *(float *)(object + offset);
    return value.u;
}

static char *marker_at(int index)
{
    return (char *)sla_marker_table + index * STRIDE;
}

static void set_marker(int index, int radius, int value, int x, int y)
{
    char *marker;

    marker = marker_at(index);
    set_int(marker, 0x00, radius);
    set_int(marker, 0x10, value);
    set_int(marker, 0x14, x);
    set_int(marker, 0x1c, y);
}

static void reset(void)
{
    clear_bytes(self_object, 0x900);
    clear_bytes((char *)sla_marker_table, STRIDE * MARKERS);
    sla_marker_count = 0;
    lookup_calls = 0;
    lookup_index = -1;
}

int main(void)
{
    reset();
    sla_marker_check(self_object);
    put(sla_marker_count == 0
        && int_is(self_object, 0x75c, 0)
        && int_is(self_object, 0x7c0, 0)
        && lookup_calls == 0 ? '0' : 'x');

    reset();
    sla_marker_count = 1;
    set_marker(0, 10, 7, 0, 0);
    set_int(self_object, 0x4b0, 3);
    set_int(self_object, 0x4b8, 4);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 1)
        && int_is(self_object, 0x7c0, 1)
        && int_is(self_object, 0x75c, 0)
        && float_bits_at(self_object, 0x790) == 0x40e00000
        && lookup_calls == 1
        && lookup_index == 9 ? '1' : 'x');

    reset();
    sla_marker_count = 1;
    set_marker(0, 10, 7, 0, 0);
    set_int(self_object, 0x694, 1);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 1)
        && int_is(self_object, 0x7c0, 0)
        && float_bits_at(self_object, 0x790) == 0
        && lookup_calls == 0 ? '2' : 'x');

    reset();
    sla_marker_count = 2;
    set_marker(0, 10, 1, 100, 0);
    set_marker(1, 10, 1, 0, 0);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 0)
        && int_is(self_object, 0x694 + 4, 1)
        && int_is(self_object, 0x75c, 1)
        && int_is(self_object, 0x7c0, 1) ? '3' : 'x');

    reset();
    sla_marker_count = 2;
    set_marker(0, 10, 1, 0, 100);
    set_marker(1, 10, 1, 0, 0);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 0)
        && int_is(self_object, 0x694 + 4, 1)
        && int_is(self_object, 0x75c, 1) ? '4' : 'x');

    reset();
    sla_marker_count = 2;
    set_marker(0, 5, 1, 4, 4);
    set_marker(1, 10, 1, 0, 0);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 0)
        && int_is(self_object, 0x694 + 4, 1)
        && int_is(self_object, 0x75c, 1) ? '5' : 'x');

    reset();
    sla_marker_count = 1;
    set_marker(0, 5, 0, 3, 4);
    set_int(self_object, 0x4b0, 0);
    set_int(self_object, 0x4b8, 0);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 1)
        && int_is(self_object, 0x7c0, 1)
        && int_is(self_object, 0x75c, 0)
        && float_bits_at(self_object, 0x790) == 0 ? '6' : 'x');

    reset();
    sla_marker_count = 2;
    set_marker(0, 10, 1, 100, 0);
    set_marker(1, 10, -3, 0, 0);
    set_float_bits(self_object, 0x790, 0x41200000);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694 + 4, 1)
        && int_is(self_object, 0x7c0, 1)
        && int_is(self_object, 0x75c, 1)
        && float_bits_at(self_object, 0x790) == 0x40e00000 ? '7' : 'x');

    reset();
    sla_marker_count = 2;
    set_marker(0, 10, 1, 100, 0);
    set_marker(1, 10, 1, 0, 0);
    set_int(self_object, 0x694 + 4, 1);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 0)
        && int_is(self_object, 0x694 + 4, 1)
        && int_is(self_object, 0x7c0, 0)
        && int_is(self_object, 0x75c, 1)
        && lookup_calls == 0 ? '8' : 'x');

    reset();
    sla_marker_count = 2;
    set_marker(0, 10, 1, 10, 0);
    set_marker(1, 10, 1, 0, 0);
    sla_marker_check(self_object);
    put(int_is(self_object, 0x694, 0)
        && int_is(self_object, 0x694 + 4, 1)
        && int_is(self_object, 0x75c, 1) ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
