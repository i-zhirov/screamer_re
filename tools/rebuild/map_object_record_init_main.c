/* The test harness for the rebuilt `map_object_record_init` module.
 *
 * The harness holds the slot table DAT_001d2d0 and the object_list_bind stub.
 * The stub records the table, the name, and the slot.
 *
 * The paths:
 *
 *     0  index 0 stores the index, sets the object field +0x08 to -1, and
 *        binds the slot at the table start
 *     1  index 0x4f overwrites a full record and binds the last slot
 *     2  two records in one run keep their own fields and bind two slots
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void map_object_record_init(int *record, int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_001d2d0[0x280];

static char text[8];
static int record[16];
static int bind_calls;
static void *bind_table;
static char *bind_name;
static int *bind_slot;

void object_list_bind(void *table, char *name, int *slot)
{
    bind_calls = bind_calls + 1;
    bind_table = table;
    bind_name = name;
    bind_slot = slot;
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

    for (index = 0; index < 16; index++) {
        record[index] = 0;
    }
    bind_calls = 0;
    bind_table = 0;
    bind_name = 0;
    bind_slot = 0;
}

int main(void)
{
    int value;

    value = 0;

    reset();
    map_object_record_init(record, 0);
    check(0, record[0] == 0 && record[2] == -1 && bind_calls == 1
        && bind_table == (void *)0x6530 && bind_name == (char *)0x6350
        && bind_slot == (int *)(DAT_001d2d0 + 0));

    reset();
    record[0] = 0x5a5a;
    record[2] = 0x7777;
    map_object_record_init(record, 0x4f);
    check(1, record[0] == 0x4f && record[2] == -1 && bind_calls == 1
        && bind_table == (void *)0x6530 && bind_name == (char *)0x6350
        && bind_slot == (int *)(DAT_001d2d0 + 0x4f * 8));

    reset();
    map_object_record_init(record, 7);
    map_object_record_init(record + 4, 9);
    check(2, record[0] == 7 && record[2] == -1 && record[4] == 9
        && record[6] == -1 && bind_calls == 2
        && bind_slot == (int *)(DAT_001d2d0 + 9 * 8));

    put('D');
    quit();
    return value;
}
