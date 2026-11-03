/* The test harness for the rebuilt `opponent_update` module.
 *
 * The harness holds the module flag, the record list, the camera position, the
 * frame delta, the two scale constants, the pointer table, and the three callee
 * stubs.  Each stub records its arguments.  The harness runs ten paths:
 *
 *     0  the module flag is clear: the function returns and runs no callee
 *     1  the record count is zero: the loop runs no pass
 *     2  a record in range with the argument zero: the advance and the publish
 *        run, the ready callee stays idle, and the fields and the three scaled
 *        words follow the record
 *     3  a record in range with the argument one: the ready callee also runs
 *        and the publish callee gets 0
 *     4  a record at exactly 0xcb20 from the camera on x: the record stays
 *        idle with the argument zero
 *     5  a record beyond 0xcb20 on a negative z distance: the record stays idle
 *     6  a record beyond 0xcb20 with the argument one: the record runs
 *     7  two records: the far record stays idle and the near record runs, so
 *        the loop advances by the 0xbc-byte stride
 *     8  the scale path: the truncating conversion gives -1, the wrap masks
 *        2048 to 0, and the copied float feeds the second scaled word
 *     9  a negative x distance and a positive z distance: the absolute value
 *        keeps the record in range
 *     D  the exit marker
 *
 * The constants and the expected values are set from their exact bit patterns,
 * because a float literal in this minimal image reads the constant segment,
 * which this harness does not load.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void opponent_update(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0010cf58;
int DAT_0010f178;
unsigned char DAT_0010f180[0x300];
unsigned int DAT_0010f174;
int DAT_00113ccc;
int DAT_00113cd0;
float DAT_00118e04;
float DAT_00100cd9;
float DAT_00100cdd;
unsigned int DAT_0010653c[64];

static char text[8];
static int targets[4][16];
static int ready_calls;
static int ready_state;
static void *ready_body;
static int advance_calls;
static int advance_state;
static void *advance_body;
static unsigned int advance_value;
static int publish_calls;
static void *publish_record;
static int publish_idle;

void opponent_callee_22378(unsigned int state, void *body)
{
    ready_calls = ready_calls + 1;
    ready_state = (int)state;
    ready_body = body;
}

void opponent_callee_20b1c(unsigned int state, void *body, float value)
{
    advance_calls = advance_calls + 1;
    advance_state = (int)state;
    advance_body = body;
    advance_value = *(unsigned int *)&value;
}

void opponent_callee_21b9c(void *record, int idle)
{
    publish_calls = publish_calls + 1;
    publish_record = record;
    publish_idle = idle;
}

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits(unsigned int *p, unsigned int bits)
{
    *p = bits;
}

static void set_float(char *base, int offset, unsigned int bits)
{
    *(unsigned int *)(base + offset) = bits;
}

static void set_int(char *base, int offset, int value)
{
    *(int *)(base + offset) = value;
}

static int get_int(char *base, int offset)
{
    return *(int *)(base + offset);
}

static unsigned int get_bits(char *base, int offset)
{
    return *(unsigned int *)(base + offset);
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static char *record_at(int index)
{
    return (char *)DAT_0010f180 + index * 0xbc;
}

static int target_int(int index, int offset)
{
    return targets[index][offset / 4];
}

static void reset(void)
{
    int index;

    clear_bytes((char *)DAT_0010f180, 0x300);
    clear_bytes((char *)targets, (int)sizeof(targets));
    DAT_0010cf58 = 1;
    DAT_0010f178 = 0;
    DAT_0010f174 = 0x11223344;
    DAT_00113ccc = 1000;
    DAT_00113cd0 = 2000;
    set_bits((unsigned int *)&DAT_00118e04, 0x3f000000);
    set_bits((unsigned int *)&DAT_00100cd9, 0x44800000);
    set_bits((unsigned int *)&DAT_00100cdd, 0x40490fdb);
    for (index = 0; index < 4; index++) {
        DAT_0010653c[index * 7] = (unsigned int)targets[index];
    }
    ready_calls = 0;
    ready_state = 0;
    ready_body = 0;
    advance_calls = 0;
    advance_state = 0;
    advance_body = 0;
    advance_value = 0;
    publish_calls = 0;
    publish_record = 0;
    publish_idle = 0;
}

static void fill_record(int index, int x, int z, int entry)
{
    char *record;

    record = record_at(index);
    set_float(record, 0x0c, 0x40000000);
    set_int(record, 0x18, entry);
    set_int(record, 0x20, x);
    set_int(record, 0x24, 0);
    set_int(record, 0x28, z);
    set_float(record, 0x2c, 0);
    set_float(record, 0x30, 0x12345678);
    set_float(record, 0x34, 0);
    set_int(record, 0x44, 0x0a0a0a0a);
    set_int(record, 0x54, 111);
    set_int(record, 0x58, 222);
    set_int(record, 0x5c, 333);
    set_float(record, 0x78, 0x3fc90fdb);
}

static int base_ok(void)
{
    return advance_calls == 1 && advance_state == 0x11223344;
}

int main(void)
{
    reset();
    DAT_0010cf58 = 0;
    DAT_0010f178 = 1;
    fill_record(0, 1000, 2000, 0);
    opponent_update(1);
    put(ready_calls == 0 && advance_calls == 0 && publish_calls == 0
        && get_int(record_at(0), 0x20) == 1000
        && target_int(0, 8) == 0 ? '0' : 'x');

    reset();
    DAT_0010f178 = 0;
    opponent_update(0);
    put(ready_calls == 0 && advance_calls == 0 && publish_calls == 0
        ? '1' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000, 2000, 0);
    opponent_update(0);
    put(ready_calls == 0 && base_ok() && publish_calls == 1
        && publish_record == record_at(0) && publish_idle == 1
        && ready_body == 0 && advance_body == record_at(0) + 0x44
        && advance_value == 0x3f800000
        && get_int(record_at(0), 0x20) == 111
        && get_int(record_at(0), 0x24) == 222
        && get_int(record_at(0), 0x28) == 333
        && get_bits(record_at(0), 0x30) == 0x3fc90fdb
        && target_int(0, 8) == 111
        && target_int(0, 12) == 222
        && target_int(0, 16) == 333
        && target_int(0, 20) == 0
        && target_int(0, 24) == 512
        && target_int(0, 28) == 0 ? '2' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000, 2000, 0);
    opponent_update(1);
    put(ready_calls == 1 && ready_body == record_at(0) + 0x44
        && advance_calls == 1 && publish_calls == 1
        && publish_idle == 0 ? '3' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000 + 0xcb20, 2000, 0);
    opponent_update(0);
    put(ready_calls == 0 && advance_calls == 0 && publish_calls == 0
        ? '4' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000, 2000 - 0xcb20 - 1, 0);
    opponent_update(0);
    put(ready_calls == 0 && advance_calls == 0 && publish_calls == 0
        ? '5' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000 + 0xcb20, 2000, 0);
    opponent_update(1);
    put(ready_calls == 1 && advance_calls == 1 && publish_calls == 1
        && publish_idle == 0 && target_int(0, 24) == 512 ? '6' : 'x');

    reset();
    DAT_0010f178 = 2;
    fill_record(0, 1000 + 0xcb21, 2000, 0);
    fill_record(1, 1000, 1999, 1);
    opponent_update(0);
    put(ready_calls == 0 && advance_calls == 1
        && advance_body == record_at(1) + 0x44
        && publish_calls == 1 && publish_record == record_at(1)
        && target_int(0, 8) == 0 && target_int(1, 8) == 111
        ? '7' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000, 2000, 0);
    set_float(record_at(0), 0x2c, 0xbb96cbe4);
    set_float(record_at(0), 0x34, 0x40c90fdb);
    opponent_update(0);
    put(advance_calls == 1 && publish_calls == 1
        && target_int(0, 20) == 0x7ff
        && target_int(0, 24) == 0x200
        && target_int(0, 28) == 0
        && get_bits(record_at(0), 0x30) == 0x3fc90fdb ? '8' : 'x');

    reset();
    DAT_0010f178 = 1;
    fill_record(0, 1000 + 0x1000, 2000 - 0x1000, 0);
    opponent_update(0);
    put(ready_calls == 0 && advance_calls == 1 && publish_calls == 1
        && publish_idle == 1 ? '9' : 'x');

    put('D');
    quit();
    return 0;
}
