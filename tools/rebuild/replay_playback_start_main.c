/* The test harness for the rebuilt `replay_playback_start` module.
 *
 * The harness holds the three-record ghost array at `DAT_0018e8c`, the playback
 * block, and five callee stubs.  Each stub records its arguments and returns a
 * scripted value, and the `path_playback_update` stub records its two
 * arguments.
 *
 * The harness runs seven paths:
 *
 *     0  the record is not active: no release, load, or acquire runs, the
 *        playback starts, and the record index clears
 *     1  the block flag is zero with an active record: the release path is
 *        skipped, the object loads, the slot is taken, the block flag turns
 *        one, and the playback starts
 *     2  the release returns 12: the function returns 12 at once
 *     3  the shared release returns 5: the function returns 5 at once
 *     4  the object load returns 7: the function returns 7 at once
 *     5  the shared release returns -1 and the load succeeds: the slot is
 *        taken and the playback starts
 *     6  the index 1 selects the second record: the stride and the model name
 *        use the second record, and the first record stays
 *     D  the exit marker
 */
#pragma off (check_stack)

struct ghost_record {
    int count;
    int index;
    int reserved_08;
    int active;
    int reserved_10;
    int reserved_14;
    int model;
};

struct playback_block {
    int entry;
    int reserved_04;
    struct ghost_record *state;
    int flag;
};

int replay_playback_start(struct playback_block *self, int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0018e8c[0x24 * 3];

static struct playback_block block;

static int release_calls;
static void *release_table;
static void *release_array;
static int *release_entry;
static int release_result;

static int shared_calls;
static void *shared_table;
static int *shared_entry;
static int shared_result;

static int load_calls;
static void *load_table;
static char *load_name;
static int *load_slot;
static int load_result;

static int acquire_calls;
static void *acquire_table;
static void *acquire_array;
static int *acquire_entry;

static int playback_calls;
static char *playback_out;
static int playback_flag;

static char text[8];

static struct ghost_record *record_at(int index)
{
    return (struct ghost_record *)(DAT_0018e8c + index * 0x24);
}

int object_release_entry(void *table, void *array, int *entry)
{
    release_calls++;
    release_table = table;
    release_array = array;
    release_entry = entry;
    return release_result;
}

int func_11230(void *table, int *index)
{
    shared_calls++;
    shared_table = table;
    shared_entry = index;
    return shared_result;
}

int object_load_named(void *table, char *name, int *slot)
{
    load_calls++;
    load_table = table;
    load_name = name;
    load_slot = slot;
    return load_result;
}

int object_slot_acquire(void *table, void *array, int *entry)
{
    acquire_calls++;
    acquire_table = table;
    acquire_array = array;
    acquire_entry = entry;
    return 0;
}

void path_playback_update(char *out, int flag)
{
    playback_calls++;
    playback_out = out;
    playback_flag = flag;
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

static void report(int ok, char marker)
{
    put(ok ? marker : 'x');
}

static void reset(void)
{
    int index;
    struct ghost_record *record;

    for (index = 0; index < 0x24 * 3; index++) {
        DAT_0018e8c[index] = 0;
    }
    record = record_at(0);
    record->count = 0x10;
    record->index = 0x1111;
    record->active = 0;
    record->model = 2;
    record = record_at(1);
    record->count = 0x20;
    record->index = 0x2222;
    record->active = 0;
    record->model = 3;
    record = record_at(2);
    record->count = 0x30;
    record->index = 0x3333;
    record->active = 0;
    record->model = 4;

    block.entry = 0x1234;
    block.reserved_04 = 0x5678;
    block.state = 0;
    block.flag = 0;

    release_calls = 0;
    release_table = 0;
    release_array = 0;
    release_entry = 0;
    release_result = 0;

    shared_calls = 0;
    shared_table = 0;
    shared_entry = 0;
    shared_result = 0;

    load_calls = 0;
    load_table = 0;
    load_name = 0;
    load_slot = 0;
    load_result = 0;

    acquire_calls = 0;
    acquire_table = 0;
    acquire_array = 0;
    acquire_entry = 0;

    playback_calls = 0;
    playback_out = 0;
    playback_flag = 0;
}

static int untouched(int index, int value)
{
    return record_at(index)->index == value;
}

int main(void)
{
    struct ghost_record *record;
    int result;
    int ok;

    reset();
    record_at(0)->active = 0;
    block.flag = 1;
    result = replay_playback_start(&block, 0);
    ok = result == 0
        && block.state == record_at(0)
        && release_calls == 0
        && shared_calls == 0
        && load_calls == 0
        && acquire_calls == 0
        && playback_calls == 1
        && playback_out == (char *)&block
        && playback_flag == 1
        && record_at(0)->index == 0
        && untouched(1, 0x2222);
    report(ok, '0');

    reset();
    record_at(0)->active = 1;
    record_at(0)->model = 5;
    block.flag = 0;
    result = replay_playback_start(&block, 0);
    ok = result == 0
        && block.state == record_at(0)
        && release_calls == 0
        && shared_calls == 0
        && load_calls == 1
        && load_table == (void *)0x6530
        && load_name == (char *)(0xd25c + 5 * 0x40)
        && load_slot == (int *)&block
        && acquire_calls == 1
        && acquire_table == (void *)0x6530
        && acquire_array == (void *)0x6350
        && acquire_entry == (int *)&block
        && block.flag == 1
        && playback_calls == 1
        && playback_out == (char *)&block
        && playback_flag == 1
        && record_at(0)->index == 0;
    report(ok, '1');

    reset();
    record_at(0)->active = 1;
    block.flag = 1;
    release_result = 12;
    result = replay_playback_start(&block, 0);
    ok = result == 12
        && release_calls == 1
        && release_table == (void *)0x6530
        && release_array == (void *)0x6350
        && release_entry == (int *)&block
        && shared_calls == 0
        && load_calls == 0
        && acquire_calls == 0
        && playback_calls == 0
        && untouched(0, 0x1111);
    report(ok, '2');

    reset();
    record_at(0)->active = 1;
    block.flag = 1;
    release_result = 0;
    shared_result = 5;
    result = replay_playback_start(&block, 0);
    ok = result == 5
        && release_calls == 1
        && shared_calls == 1
        && shared_table == (void *)0x6530
        && shared_entry == (int *)&block
        && load_calls == 0
        && acquire_calls == 0
        && playback_calls == 0;
    report(ok, '3');

    reset();
    record_at(0)->active = 1;
    block.flag = 1;
    release_result = 0;
    shared_result = 0;
    load_result = 7;
    result = replay_playback_start(&block, 0);
    ok = result == 7
        && release_calls == 1
        && shared_calls == 1
        && load_calls == 1
        && acquire_calls == 0
        && playback_calls == 0
        && untouched(0, 0x1111);
    report(ok, '4');

    reset();
    record_at(0)->active = 1;
    record_at(0)->model = 6;
    block.flag = 1;
    release_result = 0;
    shared_result = -1;
    load_result = 0;
    result = replay_playback_start(&block, 0);
    ok = result == 0
        && release_calls == 1
        && shared_calls == 1
        && load_calls == 1
        && load_name == (char *)(0xd25c + 6 * 0x40)
        && acquire_calls == 1
        && acquire_entry == (int *)&block
        && block.flag == 1
        && playback_calls == 1
        && record_at(0)->index == 0;
    report(ok, '5');

    reset();
    record_at(1)->active = 1;
    record_at(1)->model = 7;
    block.flag = 0;
    result = replay_playback_start(&block, 1);
    ok = result == 0
        && block.state == record_at(1)
        && load_calls == 1
        && load_name == (char *)(0xd25c + 7 * 0x40)
        && acquire_calls == 1
        && record_at(1)->index == 0
        && untouched(0, 0x1111)
        && untouched(2, 0x3333);
    report(ok, '6');

    put('D');
    quit();
    return 0;
}
