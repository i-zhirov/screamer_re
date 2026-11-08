/* The test harness for the rebuilt `traffic_step` module.
 *
 * The harness holds up to four 0x2a8-byte traffic records, the frame counter,
 * the saved phase, the shared counter, and the two callees that the original
 * reaches through 0x1ec7c, 0x1fa04, 0x1eed8, and 0x11850.  The elapsed script
 * sets the result of the frame comparison, so each path reaches one branch.
 *
 * The paths:
 *
 *     0  two negative records: the clear sets both finished fields to zero
 *     1  one open record with the limit 3: the fifth timeout marks it -2,
 *        binds the two slots, and restores the saved phase
 *     2  flag bit 1 sets the record state to -3
 *     3  the link callee sets the finished field: the record stays open
 *     4  the limit 0 returns after one open scan
 *     5  the first timeout keeps the branch below the limit: the negative
 *        record ends the wait
 *     6  two records: the second record takes the -2 mark after one retry
 *     7  the zero count: the scan ends at once
 *     D  the exit marker
 */
#pragma off (check_stack)

void traffic_step(int limit);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define RECORD_SIZE 0x2a8
#define RECORD_COUNT 4
#define STATE_OFFSET 0x00
#define FLAGS_OFFSET 0x16
#define FINISHED_OFFSET 0x60
#define BODY_OFFSET 0x64
#define PART_OFFSET 0x6c

char DAT_00113cd8[RECORD_SIZE * RECORD_COUNT];
int DAT_000007fd;
int DAT_000007f9;
int DAT_0000634c;
int DAT_00015218;

static char text[8];
static unsigned short elapsed_script[16];
static int elapsed_size;
static int elapsed_at;
static int reset_calls;
static int link_calls;
static int link_finish_record;
static int bind_calls;
static int bind_table;
static int bind_state;
static char *bind_first_slot;
static char *bind_last_slot;

static void set_int(int record, int offset, int value)
{
    *(int *)(DAT_00113cd8 + record * RECORD_SIZE + offset) = value;
}

static int get_int(int record, int offset)
{
    return *(int *)(DAT_00113cd8 + record * RECORD_SIZE + offset);
}

static void set_byte(int record, int offset, int value)
{
    DAT_00113cd8[record * RECORD_SIZE + offset] = (char)value;
}

unsigned short traffic_elapsed(int previous, int current)
{
    (void)previous;
    (void)current;
    if (elapsed_at < elapsed_size) {
        return elapsed_script[elapsed_at++];
    }
    return 0;
}

void traffic_reset(void)
{
    reset_calls++;
}

void traffic_link(void)
{
    link_calls++;
    if (link_finish_record >= 0) {
        set_int(link_finish_record, FINISHED_OFFSET, 1);
    }
}

int traffic_bind(int table, int state, void *slot)
{
    if (bind_calls == 0) {
        bind_first_slot = (char *)slot;
    }
    bind_calls++;
    bind_table = table;
    bind_state = state;
    bind_last_slot = (char *)slot;
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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void setup(void)
{
    int index;

    for (index = 0; index < (int)sizeof(DAT_00113cd8); index++) {
        DAT_00113cd8[index] = 0;
    }
    DAT_000007fd = 0x40;
    DAT_000007f9 = 0;
    DAT_0000634c = 0;
    DAT_00015218 = 0;
    elapsed_size = 0;
    elapsed_at = 0;
    reset_calls = 0;
    link_calls = 0;
    link_finish_record = -1;
    bind_calls = 0;
    bind_table = 0;
    bind_state = 0;
    bind_first_slot = 0;
    bind_last_slot = 0;
}

static void script(const unsigned short *values, int count)
{
    int index;

    elapsed_size = count;
    elapsed_at = 0;
    for (index = 0; index < count; index++) {
        elapsed_script[index] = values[index];
    }
}

static void script5(unsigned short a, unsigned short b, unsigned short c,
                   unsigned short d, unsigned short e, int count)
{
    elapsed_script[0] = a;
    elapsed_script[1] = b;
    elapsed_script[2] = c;
    elapsed_script[3] = d;
    elapsed_script[4] = e;
    elapsed_size = count;
    elapsed_at = 0;
}

int main(void)
{
    setup();
    DAT_00015218 = 2;
    set_int(0, STATE_OFFSET, -1);
    set_int(0, FINISHED_OFFSET, 7);
    set_int(1, STATE_OFFSET, -1);
    set_int(1, FINISHED_OFFSET, 9);
    script5(0, 0, 0, 0, 0, 1);
    traffic_step(0);
    check('0', get_int(0, FINISHED_OFFSET) == 0
        && get_int(1, FINISHED_OFFSET) == 0 && bind_calls == 0);

    setup();
    DAT_00015218 = 1;
    set_int(0, STATE_OFFSET, 0);
    set_byte(0, FLAGS_OFFSET, 0);
    set_int(0, FINISHED_OFFSET, 0);
    DAT_000007f9 = 0x1234;
    script5(0, 0x60, 0x60, 0x60, 0x60, 5);
    traffic_step(3);
    check('1', get_int(0, STATE_OFFSET) == -2 && bind_calls == 2
        && bind_table == 0x6530 && bind_state == 0x6350
        && bind_first_slot == DAT_00113cd8 + BODY_OFFSET
        && bind_last_slot == DAT_00113cd8 + PART_OFFSET
        && DAT_000007f9 == 0x1234 && reset_calls == 5);

    setup();
    DAT_00015218 = 1;
    set_int(0, STATE_OFFSET, 0);
    set_byte(0, FLAGS_OFFSET, 2);
    set_int(0, FINISHED_OFFSET, 0);
    script5(0, 0x60, 0, 0, 0, 2);
    traffic_step(1);
    check('2', get_int(0, STATE_OFFSET) == -3 && bind_calls == 0
        && reset_calls == 2);

    setup();
    DAT_00015218 = 1;
    set_int(0, STATE_OFFSET, 0);
    set_byte(0, FLAGS_OFFSET, 0);
    set_int(0, FINISHED_OFFSET, 0);
    link_finish_record = 0;
    script5(0, 0, 0, 0, 0, 1);
    traffic_step(0);
    check('3', get_int(0, FINISHED_OFFSET) == 1
        && get_int(0, STATE_OFFSET) == 0 && bind_calls == 0);

    setup();
    DAT_00015218 = 1;
    set_int(0, STATE_OFFSET, 0);
    set_byte(0, FLAGS_OFFSET, 0);
    set_int(0, FINISHED_OFFSET, 0);
    script5(0, 0, 0, 0, 0, 1);
    traffic_step(0);
    check('4', get_int(0, STATE_OFFSET) == 0 && bind_calls == 0
        && reset_calls == 1);

    setup();
    DAT_00015218 = 1;
    set_int(0, STATE_OFFSET, -1);
    set_int(0, FINISHED_OFFSET, 0);
    script5(0x60, 0, 0, 0, 0, 1);
    traffic_step(1);
    check('5', reset_calls == 2 && bind_calls == 0 && link_calls == 1);

    setup();
    DAT_00015218 = 2;
    set_int(0, STATE_OFFSET, -2);
    set_int(1, STATE_OFFSET, 0);
    set_byte(1, FLAGS_OFFSET, 0);
    set_int(1, FINISHED_OFFSET, 0);
    script5(0, 0x60, 0x60, 0, 0, 3);
    traffic_step(1);
    check('6', get_int(1, STATE_OFFSET) == -2 && bind_calls == 2
        && reset_calls == 3);

    setup();
    DAT_00015218 = 0;
    script5(0, 0, 0, 0, 0, 1);
    traffic_step(0);
    check('7', bind_calls == 0 && link_calls == 1);

    put('D');
    quit();
    return 0;
}
