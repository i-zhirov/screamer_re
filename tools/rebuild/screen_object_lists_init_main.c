/* The test harness for the rebuilt `screen_object_lists_init` module.
 *
 * The harness holds the three object record tables and the two callee stubs.
 * The `map_object_record_init` stub records the record pointer and the index.
 * The `object_list_bind` stub records the table, the name, and the record.
 *
 * The harness runs three paths:
 *
 *     0  the first run covers the 0x50 map object records: the record pointer
 *        and the index of every call
 *     1  the first run covers the 0xe car records and the 0xc track records:
 *        the record pointer, the table 0x6530, and the name 0x6350 of every
 *        bind
 *     2  a second run adds the same calls again, so the total count doubles
 *        and the second block repeats the first block
 *     D  the exit marker
 *
 * The marker goes through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void screen_object_lists_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0011db78[0x50 * 0x64];
char DAT_0011d550[0xe * 0x44];
char DAT_0011d908[0xc * 0x34];

static char text[8];
static char *init_record[160];
static int init_index[160];
static int init_calls;
static void *bind_table[64];
static char *bind_name[64];
static void *bind_record[64];
static int bind_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void map_object_record_init(char *record, int index)
{
    if (init_calls < 160) {
        init_record[init_calls] = record;
        init_index[init_calls] = index;
    }
    init_calls = init_calls + 1;
}

void object_list_bind(void *table, char *name, void *record)
{
    if (bind_calls < 64) {
        bind_table[bind_calls] = table;
        bind_name[bind_calls] = name;
        bind_record[bind_calls] = record;
    }
    bind_calls = bind_calls + 1;
}

static int check_map_records(int start)
{
    int index;

    for (index = 0; index < 0x50; index++) {
        if (init_record[start + index] != DAT_0011db78 + index * 0x64)
            return 0;
        if (init_index[start + index] != index)
            return 0;
    }
    return 1;
}

static int check_binds(int start)
{
    int index;

    for (index = 0; index < 0xe; index++) {
        if (bind_table[start + index] != (void *)0x6530)
            return 0;
        if (bind_name[start + index] != (char *)0x6350)
            return 0;
        if (bind_record[start + index] != DAT_0011d550 + index * 0x44)
            return 0;
    }
    for (index = 0; index < 0xc; index++) {
        if (bind_table[start + 0xe + index] != (void *)0x6530)
            return 0;
        if (bind_name[start + 0xe + index] != (char *)0x6350)
            return 0;
        if (bind_record[start + 0xe + index] != DAT_0011d908 + index * 0x34)
            return 0;
    }
    return 1;
}

int main(void)
{
    int value;

    value = 0;

    screen_object_lists_init();
    put(init_calls == 0x50 && check_map_records(0) ? '0' : 'x');
    put(bind_calls == 0x1a && check_binds(0) ? '1' : 'x');

    screen_object_lists_init();
    put(init_calls == 0xa0 && bind_calls == 0x34
        && check_map_records(0x50) && check_binds(0x1a) ? '2' : 'x');

    put('D');
    quit();
    return value;
}
