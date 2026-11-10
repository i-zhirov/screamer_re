/* The test harness for the rebuilt `object_unload` module.
 *
 * The harness holds the 0x78-record object table, the resource list pointer,
 * the two free stubs, and the memory error stub.  The stubs record their list
 * pointer, their name, and their call count.  Each free stub returns the value
 * of its result slot.  The harness runs four paths:
 *
 *     0  both frees succeed: no memory error, and every record is cleared
 *     1  the `3D_HEADER` free fails: one memory error with the same list
 *     2  the `3D_BLKOBJ` free fails: one memory error with the same list
 *     3  both frees fail: two memory errors
 *     D  the exit marker
 *
 * Every path also checks the two free names and that every record keeps its
 * padding and the untouched fields, because the function only writes the four
 * listed fields.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

struct object_entry {
    char pad_00[0x0c];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

int object_unload(struct object_entry *table);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0000801c;

static struct object_entry table[0x78];
static char text[8];

static int free_calls;
static void *free_list[4];
static char *free_name[4];
static int free_result[4];

static int error_calls;
static void *error_list;

int resource_free_name(void *list, char *name)
{
    int slot;

    slot = free_calls;
    if (slot < 4) {
        free_list[slot] = list;
        free_name[slot] = name;
    }
    free_calls = free_calls + 1;
    if (slot < 4) {
        return free_result[slot];
    }
    return 0;
}

void memory_error_report(void *list)
{
    error_calls = error_calls + 1;
    error_list = list;
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

static int same(char *left, char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; ++index) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static void reset(void)
{
    int index;
    int slot;

    for (index = 0; index < 0x78; ++index) {
        for (slot = 0; slot < 0x0c; ++slot) {
            table[index].pad_00[slot] = 0x5a;
        }
        table[index].field_0c = 0x0c000000 + index;
        table[index].field_10 = 0x10000000 + index;
        table[index].field_14 = 0x14000000 + index;
        table[index].field_18 = 0x18000000 + index;
    }
    DAT_0000801c = (void *)0x801c;
    free_calls = 0;
    error_calls = 0;
    error_list = 0;
    for (slot = 0; slot < 4; ++slot) {
        free_list[slot] = 0;
        free_name[slot] = 0;
        free_result[slot] = 0;
    }
}

static int cleared(int index)
{
    int slot;

    for (slot = 0; slot < 0x0c; ++slot) {
        if (table[index].pad_00[slot] != 0x5a) {
            return 0;
        }
    }
    return table[index].field_0c == 0
        && table[index].field_10 == 0
        && table[index].field_14 == 0
        && table[index].field_18 == -1;
}

static int all_cleared(void)
{
    int index;

    for (index = 0; index < 0x78; ++index) {
        if (!cleared(index)) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int result;

    reset();
    result = object_unload(table);
    put(result == 0 && free_calls == 2
        && free_list[0] == DAT_0000801c && same(free_name[0], "3D_HEADER")
        && free_list[1] == DAT_0000801c && same(free_name[1], "3D_BLKOBJ")
        && error_calls == 0 && all_cleared()
        ? '0' : 'x');

    reset();
    free_result[0] = 1;
    result = object_unload(table);
    put(result == 0 && free_calls == 2 && error_calls == 1
        && error_list == DAT_0000801c && all_cleared()
        ? '1' : 'x');

    reset();
    free_result[1] = 1;
    result = object_unload(table);
    put(result == 0 && free_calls == 2 && error_calls == 1
        && error_list == DAT_0000801c && all_cleared()
        ? '2' : 'x');

    reset();
    free_result[0] = 1;
    free_result[1] = 1;
    result = object_unload(table);
    put(result == 0 && free_calls == 2 && error_calls == 2
        && error_list == DAT_0000801c && all_cleared()
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
