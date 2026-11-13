/* The test harness for the rebuilt `object_handle_release` module.
 *
 * The harness holds the object entry, the resource list pointer, and the
 * `resource_find` stub.  The stub records the call order, the context, and the
 * key.  The harness runs four paths:
 *
 *     0  a zero first handle: the function returns 9, calls no helper, and
 *        leaves the entry unchanged
 *     1  a non-zero first handle and a field `+0x18` that is not -1: the
 *        function returns 0xb, calls no helper, and leaves the entry unchanged
 *     2  a non-zero first handle and the field `+0x18` at -1: the function
 *        frees the first handle, clears the flag and the three fields, keeps
 *        the field `+0x18`, and returns 0
 *     3  a sentinel context and another first handle: the function frees the
 *        handle once with that context, clears the fields, and returns 0
 *     D  the exit marker
 *
 * Every path checks the entry bytes, the call count, the context, and the key,
 * because the original reads the entry and frees the first handle.
 */
#pragma off (check_stack)

struct object_entry {
    unsigned char flag;
    char pad_01[0x0b];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
};

int object_handle_release(struct object_entry *entry);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

#define MAX_CALLS 1

int DAT_0000801c;

static struct object_entry entry;
static char text[8];

static int find_calls;
static int find_context[MAX_CALLS];
static int find_key[MAX_CALLS];

int resource_find(int context, int key)
{
    int slot;

    slot = find_calls;
    if (slot < MAX_CALLS) {
        find_context[slot] = context;
        find_key[slot] = key;
    }
    find_calls = slot + 1;
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

static void reset(void)
{
    int slot;

    entry.flag = 0x5a;
    for (slot = 0; slot < 0x0b; ++slot) {
        entry.pad_01[slot] = 0x5b;
    }
    entry.field_0c = 0x0c0c0c0c;
    entry.field_10 = 0x10101010;
    entry.field_14 = 0x14141414;
    entry.field_18 = 0x18181818;
    DAT_0000801c = 0x001c801c;
    find_calls = 0;
    find_context[0] = 0;
    find_key[0] = 0;
}

static int pad_is_sentinel(void)
{
    int slot;

    for (slot = 0; slot < 0x0b; ++slot) {
        if (entry.pad_01[slot] != 0x5b) {
            return 0;
        }
    }
    return 1;
}

static int entry_unchanged(int first, int field_10, int second, int field_18)
{
    return entry.flag == 0x5a
        && pad_is_sentinel()
        && entry.field_0c == first
        && entry.field_10 == field_10
        && entry.field_14 == second
        && entry.field_18 == field_18;
}

static int entry_cleared(int field_18)
{
    return entry.flag == 0
        && pad_is_sentinel()
        && entry.field_0c == 0
        && entry.field_10 == 0
        && entry.field_14 == 0
        && entry.field_18 == field_18;
}

int main(void)
{
    int result;

    reset();
    entry.field_0c = 0;
    entry.field_18 = -1;
    result = object_handle_release(&entry);
    put(result == 9 && find_calls == 0
        && entry_unchanged(0, 0x10101010, 0x14141414, -1)
        ? '0' : 'x');

    reset();
    entry.field_0c = 0x11111111;
    entry.field_18 = 0;
    result = object_handle_release(&entry);
    put(result == 0xb && find_calls == 0
        && entry_unchanged(0x11111111, 0x10101010, 0x14141414, 0)
        ? '1' : 'x');

    reset();
    entry.field_0c = 0x22222222;
    entry.field_18 = -1;
    result = object_handle_release(&entry);
    put(result == 0 && find_calls == 1
        && find_context[0] == 0x001c801c && find_key[0] == 0x22222222
        && entry_cleared(-1)
        ? '2' : 'x');

    reset();
    DAT_0000801c = 0x12345678;
    entry.field_0c = 0x44444444;
    entry.field_18 = -1;
    result = object_handle_release(&entry);
    put(result == 0 && find_calls == 1
        && find_context[0] == 0x12345678 && find_key[0] == 0x44444444
        && entry_cleared(-1)
        ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
