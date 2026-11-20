/* The test harness for the rebuilt `sound_variant_lookup` module.
 *
 * The harness holds the sound state `DAT_0001d2c8`, the table flag
 * `DAT_00008058`, and the sample base `DAT_0001906c`.  The
 * `sound_variant_select` stub records the effect and the mode of the stack
 * record and returns a fixed sample id.  The `table_lookup` stub records the
 * index and returns a fixed table entry.
 *
 * The paths:
 *
 *     0  the zero sound state returns -1 and calls nothing
 *     1  the zero table flag returns -1 and calls nothing
 *     2  the base adds to the selected sample id and the entry returns
 *     3  the mode 3 and the base 0 pass through
 *     4  the selected -1 with the base 5 gives the index 4
 *     5  a large base and a negative effect pass through
 *     6  the effect 0 and the mode 0 at the base 0
 *     7  a non-zero state and flag pass the gates
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

struct sound_entry {
    int effect;
    int mode;
};

int sound_variant_lookup(int effect, int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0001d2c8;
int DAT_00008058;
int DAT_0001906c;

static char text[8];
static int select_calls;
static int select_effect;
static int select_mode;
static int select_result;
static int lookup_calls;
static int lookup_index;
static int lookup_result;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int sound_variant_select(struct sound_entry *entry)
{
    select_calls = select_calls + 1;
    select_effect = entry->effect;
    select_mode = entry->mode;
    return select_result;
}

int table_lookup(int index)
{
    lookup_calls = lookup_calls + 1;
    lookup_index = index;
    return lookup_result;
}

static void reset(void)
{
    DAT_0001d2c8 = 1;
    DAT_00008058 = 1;
    DAT_0001906c = 0;
    select_calls = 0;
    select_effect = 0;
    select_mode = 0;
    select_result = 0;
    lookup_calls = 0;
    lookup_index = 0;
    lookup_result = 0;
}

int main(void)
{
    int result;

    reset();
    DAT_0001d2c8 = 0;
    result = sound_variant_lookup(5, 2);
    put(result == -1 && select_calls == 0 && lookup_calls == 0 ? '0' : 'x');

    reset();
    DAT_00008058 = 0;
    result = sound_variant_lookup(5, 2);
    put(result == -1 && select_calls == 0 && lookup_calls == 0 ? '1' : 'x');

    reset();
    DAT_0001906c = 100;
    select_result = 7;
    lookup_result = 4242;
    result = sound_variant_lookup(5, 2);
    put(result == 4242 && select_calls == 1 && select_effect == 5
        && select_mode == 2 && lookup_calls == 1
        && lookup_index == 107 ? '2' : 'x');

    reset();
    select_result = 40;
    lookup_result = -9;
    result = sound_variant_lookup(1, 3);
    put(result == -9 && select_effect == 1 && select_mode == 3
        && lookup_index == 40 ? '3' : 'x');

    reset();
    DAT_0001906c = 5;
    select_result = -1;
    lookup_result = -7;
    result = sound_variant_lookup(9, 0);
    put(result == -7 && lookup_index == 4 ? '4' : 'x');

    reset();
    DAT_0001906c = 0x1000;
    select_result = 3;
    lookup_result = 0x55;
    result = sound_variant_lookup(-2, 1);
    put(result == 0x55 && select_effect == -2
        && lookup_index == 0x1003 ? '5' : 'x');

    reset();
    select_result = 0;
    lookup_result = 1;
    result = sound_variant_lookup(0, 0);
    put(result == 1 && lookup_index == 0 ? '6' : 'x');

    reset();
    DAT_0001d2c8 = 0x3333;
    DAT_00008058 = -1;
    DAT_0001906c = 2;
    select_result = 6;
    lookup_result = 0x7f;
    result = sound_variant_lookup(4, 2);
    put(result == 0x7f && select_calls == 1 && lookup_index == 8
        ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
