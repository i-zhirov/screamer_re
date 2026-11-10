/* The test harness for the rebuilt `traffic_reset` module.
 *
 * The harness holds four 0x2a8-byte traffic records at 0x13cd8, the player
 * index at 0x1d250, the record count at 0x15218, and the shared counter at
 * 0x634c.  The `player_state_snapshot` stub records its argument.  The
 * `traffic_register` stub records every call, its five arguments, and a
 * scripted zero return, so the retry path is visible.
 *
 * The harness runs five paths:
 *
 *     0  a zero count: one snapshot, no register call, and the player record
 *        state takes -1
 *     1  the count 1 and the player index 0: the one record is skipped
 *     2  the count 3 and the player index 1: the records 0 and 2 take two
 *        register calls with the shadow record 0x10100, the size 0xc, the name
 *        0x1010c, and the value 0x74
 *     3  the count 2 and the player index 0, with two zero returns: the third
 *        call for the index 1 returns non-zero and the counter advances
 *     4  the count 4 and the player index 3: the records 0, 1, and 2 take three
 *        register calls in order
 *     D  the exit marker
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x2a8
#define RECORD_COUNT 4
#define CALL_MAX 8

void traffic_reset(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d250;
int DAT_00115218;
int DAT_0000634c;
unsigned char DAT_00113cd8[RECORD_SIZE * RECORD_COUNT];

static int snapshot_calls;
static void *snapshot_arg;
static int register_calls;
static int register_zero_left;
static void *register_record[CALL_MAX];
static int register_size[CALL_MAX];
static void *register_name[CALL_MAX];
static int register_value[CALL_MAX];
static int register_index[CALL_MAX];
static char text[8];

void player_state_snapshot(void *object)
{
    snapshot_calls++;
    snapshot_arg = object;
}

int traffic_register(void *record, int size, void *name, int value, int index)
{
    if (register_calls < CALL_MAX) {
        register_record[register_calls] = record;
        register_size[register_calls] = size;
        register_name[register_calls] = name;
        register_value[register_calls] = value;
        register_index[register_calls] = index;
    }
    register_calls++;
    if (register_zero_left > 0) {
        register_zero_left--;
        return 0;
    }
    return 1;
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

static int record_state(int index)
{
    return *(int *)(DAT_00113cd8 + index * RECORD_SIZE);
}

static void reset(void)
{
    int index;

    for (index = 0; index < RECORD_SIZE * RECORD_COUNT; index++) {
        DAT_00113cd8[index] = 0;
    }
    for (index = 0; index < CALL_MAX; index++) {
        register_record[index] = 0;
        register_size[index] = 0;
        register_name[index] = 0;
        register_value[index] = 0;
        register_index[index] = 0;
    }
    DAT_0011d250 = 0;
    DAT_00115218 = 0;
    DAT_0000634c = 0;
    snapshot_calls = 0;
    snapshot_arg = 0;
    register_calls = 0;
    register_zero_left = 0;
}

static int call_ok(int slot, int index)
{
    return register_record[slot] == (void *)0x10100
        && register_size[slot] == 0xc
        && register_name[slot] == (void *)0x1010c
        && register_value[slot] == 0x74
        && register_index[slot] == index;
}

int main(void)
{
    int ok;
    int index;

    reset();
    DAT_00115218 = 0;
    DAT_0011d250 = 0;
    traffic_reset();
    ok = snapshot_calls == 1 && snapshot_arg == (void *)0xf91c
        && register_calls == 0 && DAT_0000634c == 0
        && record_state(0) == -1 && record_state(1) == 0;
    report(ok, '0');

    reset();
    DAT_00115218 = 1;
    DAT_0011d250 = 0;
    traffic_reset();
    ok = snapshot_calls == 1 && register_calls == 0
        && DAT_0000634c == 1
        && record_state(0) == -1 && record_state(1) == 0;
    report(ok, '1');

    reset();
    DAT_00115218 = 3;
    DAT_0011d250 = 1;
    traffic_reset();
    ok = snapshot_calls == 1 && register_calls == 2
        && call_ok(0, 0) && call_ok(1, 2)
        && DAT_0000634c == 3
        && record_state(0) == 0 && record_state(1) == -1
        && record_state(2) == 0;
    report(ok, '2');

    reset();
    DAT_00115218 = 2;
    DAT_0011d250 = 0;
    register_zero_left = 2;
    traffic_reset();
    ok = snapshot_calls == 1 && register_calls == 3
        && call_ok(0, 1) && call_ok(1, 1) && call_ok(2, 1)
        && DAT_0000634c == 2
        && record_state(0) == -1;
    report(ok, '3');

    reset();
    DAT_00115218 = 4;
    DAT_0011d250 = 3;
    traffic_reset();
    ok = snapshot_calls == 1 && register_calls == 3
        && call_ok(0, 0) && call_ok(1, 1) && call_ok(2, 2)
        && DAT_0000634c == 4
        && record_state(3) == -1;
    report(ok, '4');

    put('D');
    quit();
    return 0;
}
