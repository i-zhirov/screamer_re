/* The test harness for the rebuilt `object_handle_table_release` module.
 *
 * The harness holds the resource list pointer, the free stub, and the memory
 * error stub.  The free stub records its list pointer, its name, and its call
 * count, and it returns the value of its result slot.  The error stub records
 * its list pointer and its call count.
 *
 * The harness runs three paths:
 *
 *     0  the free succeeds: one free with the list and the `OBJ_LIST` name,
 *        and no memory error
 *     1  the free fails: one free with the same arguments, and one memory
 *        error with the same list
 *     2  a second list and a second failed free: the error receives that list
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

void object_handle_table_release(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_0000801c;

static char text[8];

static int free_calls;
static void *free_list[2];
static char *free_name[2];
static int free_result[2];

static int error_calls;
static void *error_list;

int resource_free_name(void *list, char *name)
{
    int slot;

    slot = free_calls;
    if (slot < 2) {
        free_list[slot] = list;
        free_name[slot] = name;
    }
    free_calls = free_calls + 1;
    if (slot < 2) {
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
    int slot;

    DAT_0000801c = (void *)0x801c;
    free_calls = 0;
    error_calls = 0;
    error_list = 0;
    for (slot = 0; slot < 2; ++slot) {
        free_list[slot] = 0;
        free_name[slot] = 0;
        free_result[slot] = 0;
    }
}

int main(void)
{
    reset();
    object_handle_table_release();
    put(free_calls == 1
        && free_list[0] == DAT_0000801c && same(free_name[0], "OBJ_LIST")
        && error_calls == 0
        ? '0' : 'x');

    reset();
    free_result[0] = 1;
    object_handle_table_release();
    put(free_calls == 1
        && free_list[0] == DAT_0000801c && same(free_name[0], "OBJ_LIST")
        && error_calls == 1 && error_list == DAT_0000801c
        ? '1' : 'x');

    reset();
    DAT_0000801c = (void *)0x12345678;
    free_result[0] = 0x20;
    object_handle_table_release();
    put(free_calls == 1
        && free_list[0] == DAT_0000801c && same(free_name[0], "OBJ_LIST")
        && error_calls == 1 && error_list == DAT_0000801c
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
