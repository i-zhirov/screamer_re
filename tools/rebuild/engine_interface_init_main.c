/* The test harness for the rebuilt `engine_interface_init` module.
 *
 * The harness holds the list pointer, the name buffer, the record table, the
 * state word, and the two callee stubs.  The stubs record their arguments and
 * return the chosen pointers.  The harness runs two paths:
 *
 *     0  the successful allocation stores the list result at DAT_0000801c,
 *        clears the fields +0x0c, +0x10, and +0x14, sets the field +0x18 to
 *        -1, keeps the other fields, clears the name buffer, and clears the
 *        state word
 *     1  the failed allocation keeps the list stub unrun, stores 0 at
 *        DAT_0000801c, and still clears the records, the name buffer, and the
 *        state word
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void engine_interface_init(void *records, int *state);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct interface_record {
    int field_00;
    int field_04;
    int field_08;
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

int DAT_0000801c;
char DAT_00007fe8;

static struct interface_record records[0x78];
static char text[8];
static unsigned int alloc_size;
static void *alloc_result;
static void *list_list;
static int list_count;
static void *list_result;
static int alloc_calls;
static int list_calls;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void *memory_alloc(unsigned int size)
{
    alloc_size = size;
    alloc_calls = alloc_calls + 1;
    return alloc_result;
}

void *list_create(void *list, int count)
{
    list_list = list;
    list_count = count;
    list_calls = list_calls + 1;
    return list_result;
}

static void fill(void)
{
    int index;

    for (index = 0; index < 0x78; ++index) {
        records[index].field_00 = 0x55;
        records[index].field_04 = 0x55;
        records[index].field_08 = 0x55;
        records[index].field_0c = 0x55;
        records[index].field_10 = 0x55;
        records[index].field_14 = 0x55;
        records[index].field_18 = 0x55;
    }
    DAT_0000801c = 0x55;
    DAT_00007fe8 = 'A';
    alloc_size = 0;
    alloc_calls = 0;
    list_calls = 0;
    list_list = 0;
    list_count = 0;
}

static int cleared(void)
{
    int index;

    for (index = 0; index < 0x78; ++index) {
        if (records[index].field_00 != 0x55
            || records[index].field_04 != 0x55
            || records[index].field_08 != 0x55
            || records[index].field_0c != 0
            || records[index].field_10 != 0
            || records[index].field_14 != 0
            || records[index].field_18 != -1) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int state;

    fill();
    state = 0x12345678;
    alloc_result = (void *)0x1111;
    list_result = (void *)0x2222;
    engine_interface_init(records, &state);
    put(alloc_calls == 1 && alloc_size == 0x4c
        && list_calls == 1 && list_list == (void *)0x1111
        && list_count == 0xb4
        && DAT_0000801c == (int)0x2222
        && cleared() && DAT_00007fe8 == 0 && state == 0 ? '0' : 'x');

    fill();
    state = 0x12345678;
    alloc_result = 0;
    list_result = (void *)0x2222;
    engine_interface_init(records, &state);
    put(alloc_calls == 1 && alloc_size == 0x4c
        && list_calls == 0
        && DAT_0000801c == 0
        && cleared() && DAT_00007fe8 == 0 && state == 0 ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
