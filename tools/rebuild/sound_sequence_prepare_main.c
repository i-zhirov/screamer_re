/* The test harness for the rebuilt `sound_sequence_prepare` module.
 *
 * The harness holds the row array, the row count, the output cursor, the four
 * scratch globals, and the two callee stubs.  The `sound_range_update` stub
 * sets the row array and returns the row count.  The `sound_source_collect`
 * stub records the region pointer and the row limit and advances the output
 * cursor by a chosen number of 8-byte records.
 *
 * The harness runs five paths:
 *
 *     0  one row entry and the count 0: the loop runs once, the call receives
 *        the entry plus the fill value 0xaaaabbbb, the limit equals the fill
 *        value, and the active count is 1
 *     1  three row entries with the count 2: the middle entry is -1, so the
 *        loop runs three times and makes two calls, and the active count is 2
 *     2  the -1 entry with the count 0: the loop runs once and makes no call,
 *        so the limit keeps its value and the active count is 0
 *     3  two row entries and two records per call: the output advances by
 *        four records, so the active count is 4
 *     4  the count -1: the loop still runs once, so the active count is 1
 *     D  the exit marker
 */
#pragma off (check_stack)

int sound_sequence_prepare(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 8

int *grid_row;
int DAT_000734f0;
int DAT_000734f4;
int DAT_000734fc;
int DAT_00072488;

static char text[8];
static int row[8];
static int row_count;
static int collect_records;
static int call_count;
static int *call_regions[MAX_CALLS];
static int call_limit[MAX_CALLS];

int sound_range_update(void)
{
    grid_row = row;
    return row_count;
}

int *sound_source_collect(int *regions, int *output)
{
    int index;

    index = call_count;
    if (index < MAX_CALLS) {
        call_regions[index] = regions;
        call_limit[index] = DAT_000734f4;
    }
    call_count = index + 1;
    return output + collect_records * 2;
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

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

static void reset(void)
{
    int index;

    DAT_000734f0 = 0;
    DAT_000734f4 = 0x55;
    DAT_000734fc = 0x77;
    DAT_00072488 = 0x99;
    row_count = 0;
    collect_records = 1;
    call_count = 0;
    for (index = 0; index < 8; index++) {
        row[index] = -1;
    }
    for (index = 0; index < MAX_CALLS; index++) {
        call_regions[index] = 0;
        call_limit[index] = 0;
    }
}

int main(void)
{
    reset();
    row[0] = 0x1111;
    row_count = 0;
    sound_sequence_prepare();
    check(0, DAT_000734f0 == -1
        && DAT_000734fc == 0
        && DAT_000734f4 == 0xaaaabbbb
        && call_count == 1
        && call_regions[0] == (int *)(0x1111 + 0xaaaabbbb)
        && call_limit[0] == 0xaaaabbbb
        && DAT_00072488 == 1);

    reset();
    row[0] = 0x2000;
    row[1] = -1;
    row[2] = 0x3000;
    row_count = 2;
    sound_sequence_prepare();
    check(1, DAT_000734f0 == -1
        && call_count == 2
        && call_regions[0] == (int *)(0x2000 + 0xaaaabbbb)
        && call_regions[1] == (int *)(0x3000 + 0xaaaabbbb)
        && DAT_00072488 == 2);

    reset();
    row[0] = -1;
    row_count = 0;
    sound_sequence_prepare();
    check(2, DAT_000734f0 == -1
        && DAT_000734f4 == 0x55
        && call_count == 0
        && DAT_00072488 == 0);

    reset();
    row[0] = 0x4000;
    row[1] = 0x5000;
    row_count = 1;
    collect_records = 2;
    sound_sequence_prepare();
    check(3, DAT_000734f0 == -1
        && call_count == 2
        && DAT_00072488 == 4);

    reset();
    row[0] = 0x6000;
    row_count = -1;
    sound_sequence_prepare();
    check(4, DAT_000734f0 == -2
        && call_count == 1
        && DAT_00072488 == 1);

    put('D');
    quit();
    return 0;
}
