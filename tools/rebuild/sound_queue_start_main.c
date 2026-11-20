/* The test harness for the rebuilt `sound_queue_start` module.
 *
 * The harness holds the frame counter DAT_000007fd, the sample base
 * DAT_0001906c, the table id DAT_00019074, and the start frame DAT_00019078.
 * The `sound_variant_select` stub records the queue record and returns a fixed
 * sample id.  The `table_lookup` stub records the index and returns a fixed
 * table entry.
 *
 * The paths:
 *
 *     0  the selected -1 returns -1, calls no lookup, and touches no global
 *     1  a sample id and a base form the table id, and the entry returns
 *     2  the sample id 0 is valid and the base 0 keeps the table id 0
 *     3  a negative sample id with the base 10 forms the table id 8
 *     4  a large base adds to the sample id and the entry passes through
 *     5  the queue record pointer arrives unchanged
 *     6  a -1 table entry returns -1 after the two globals are set
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

int sound_queue_start(struct sound_entry *entry);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_000007fd;
int DAT_0001906c;
int DAT_00019074;
int DAT_00019078;

static char text[8];
static int select_calls;
static int select_effect;
static int select_mode;
static int select_result;
static struct sound_entry *select_entry;
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
    select_entry = entry;
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
    DAT_000007fd = 0x1234;
    DAT_0001906c = 0;
    DAT_00019074 = -7;
    DAT_00019078 = -9;
    select_calls = 0;
    select_effect = 0;
    select_mode = 0;
    select_result = 0;
    select_entry = 0;
    lookup_calls = 0;
    lookup_index = 0;
    lookup_result = 0;
}

int main(void)
{
    struct sound_entry entry;
    int result;

    entry.effect = 4;
    entry.mode = 2;

    reset();
    select_result = -1;
    result = sound_queue_start(&entry);
    put(result == -1 && select_calls == 1 && lookup_calls == 0
        && DAT_00019074 == -7 && DAT_00019078 == -9 ? '0' : 'x');

    reset();
    DAT_0001906c = 100;
    select_result = 5;
    lookup_result = 4242;
    result = sound_queue_start(&entry);
    put(result == 4242 && select_calls == 1 && select_effect == 4
        && select_mode == 2 && lookup_calls == 1 && lookup_index == 105
        && DAT_00019074 == 105 && DAT_00019078 == 0x1234 ? '1' : 'x');

    reset();
    select_result = 0;
    lookup_result = 7;
    result = sound_queue_start(&entry);
    put(result == 7 && lookup_index == 0 && DAT_00019074 == 0
        && DAT_00019078 == 0x1234 ? '2' : 'x');

    reset();
    DAT_0001906c = 10;
    select_result = -2;
    lookup_result = -9;
    result = sound_queue_start(&entry);
    put(result == -9 && lookup_index == 8 && DAT_00019074 == 8
        && DAT_00019078 == 0x1234 ? '3' : 'x');

    reset();
    DAT_0001906c = 0x1000;
    select_result = 3;
    lookup_result = 0x55;
    result = sound_queue_start(&entry);
    put(result == 0x55 && lookup_index == 0x1003 && DAT_00019074 == 0x1003
        ? '4' : 'x');

    reset();
    DAT_0001906c = 1;
    select_result = 6;
    lookup_result = 1;
    result = sound_queue_start(&entry);
    put(result == 1 && select_entry == &entry && lookup_index == 7
        ? '5' : 'x');

    reset();
    select_result = 9;
    lookup_result = -1;
    result = sound_queue_start(&entry);
    put(result == -1 && lookup_calls == 1 && DAT_00019074 == 9
        && DAT_00019078 == 0x1234 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
