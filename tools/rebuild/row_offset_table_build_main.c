/* The test harness for the rebuilt `row_offset_table_build` module.
 *
 * The harness holds the 481-entry row offset table and two guard cells.  It
 * fills the table and the guards with a sentinel before the checks, so a
 * missing store or a store past the last entry fails the check.  It resets the
 * buffer again before the two-call path.
 *
 * The paths:
 *
 *     0  every entry from index 0 to 0x1e0 equals the index times 640, and
 *        the two guard cells keep the sentinel
 *     1  the entry at index 1 holds 640 and the entry at index 2 holds 1280
 *     2  the entry at index 0x1e0 holds 307200
 *     3  two calls leave the same table and the same guards
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void row_offset_table_build(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define ENTRY_COUNT 0x1e1
#define GUARD_COUNT 2
#define SENTINEL 0xA5A5A5A5u

unsigned int row_offset_table[ENTRY_COUNT + GUARD_COUNT];

static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    for (index = 0; index < ENTRY_COUNT + GUARD_COUNT; index = index + 1) {
        row_offset_table[index] = SENTINEL;
    }
}

static int guards_ok(void)
{
    return row_offset_table[ENTRY_COUNT] == SENTINEL
        && row_offset_table[ENTRY_COUNT + 1] == SENTINEL;
}

static int verify_all(void)
{
    int index;

    for (index = 0; index < ENTRY_COUNT; index = index + 1) {
        if (row_offset_table[index] != (unsigned int)(index * 640)) {
            return 0;
        }
    }
    return guards_ok();
}

int main(void)
{
    reset();
    row_offset_table_build();
    put(verify_all() ? '0' : 'x');

    put(row_offset_table[1] == 640
        && row_offset_table[2] == 1280 ? '1' : 'x');

    put(row_offset_table[0x1e0] == 307200 ? '2' : 'x');

    reset();
    row_offset_table_build();
    row_offset_table_build();
    put(verify_all() ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
