/* The test harness for the rebuilt `overlay_queue_update` module.
 *
 * The harness holds the signed source list, the enable bytes, the pending
 * flags, the queue count, the queue table, the nine 0x18-byte records, and the
 * `table_lookup` stub.  The stub records the index and copies the queue, the
 * count, and the flag table at the moment of the call.  The harness runs seven
 * paths:
 *
 *     0  the source starts with a negative entry: the queue stays empty and
 *        the table runs no action
 *     1  a set enable byte holds the entry, and a clear enable byte moves the
 *        held entry into the queue
 *     2  six entries fill the queue: the count stops at 4, the sixth entry
 *        overwrites the fifth, and the first record assigns the value
 *     3  the fourth record holds the xor flag: the value xors through the
 *        pointer
 *     4  a full queue that matches no key: all nine records mismatch and the
 *        queue clears
 *     5  a two-entry queue that matches the key prefix: the queue stays
 *     6  a zero source entry with the flag set moves the zero into the queue
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void overlay_queue_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct queue_record {
    char key[5];
    char pad[7];
    int *value_ptr;
    int value;
    int xor_flag;
};

int DAT_00102108[16];
unsigned char DAT_00100002[16];
int DAT_00120884[16];
int DAT_0012087c;
int DAT_0012082c[8];
struct queue_record DAT_00102e04[9];

static char text[8];
static int lookup_calls;
static int lookup_index;
static int capture_count;
static int capture_queue[6];
static int capture_flags[6];
static int target;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

int table_lookup(int index)
{
    int k;

    lookup_calls = lookup_calls + 1;
    lookup_index = index;
    capture_count = DAT_0012087c;
    for (k = 0; k < 6; k++) {
        capture_queue[k] = DAT_0012082c[k];
        capture_flags[k] = DAT_00120884[k];
    }
    return 0;
}

static void set_key(int index, int a, int b, int c, int d, int e)
{
    DAT_00102e04[index].key[0] = (char)a;
    DAT_00102e04[index].key[1] = (char)b;
    DAT_00102e04[index].key[2] = (char)c;
    DAT_00102e04[index].key[3] = (char)d;
    DAT_00102e04[index].key[4] = (char)e;
}

static void set_all_keys(int a, int b, int c, int d, int e)
{
    int index;

    for (index = 0; index < 9; index++) {
        set_key(index, a, b, c, d, e);
    }
}

static void reset(void)
{
    int index;

    for (index = 0; index < 9; index++) {
        DAT_00102e04[index].key[0] = 0;
        DAT_00102e04[index].key[1] = 0;
        DAT_00102e04[index].key[2] = 0;
        DAT_00102e04[index].key[3] = 0;
        DAT_00102e04[index].key[4] = 0;
        DAT_00102e04[index].value_ptr = 0;
        DAT_00102e04[index].value = 0;
        DAT_00102e04[index].xor_flag = 0;
    }
    for (index = 0; index < 16; index++) {
        DAT_00102108[index] = -1;
        DAT_00100002[index] = 0;
        DAT_00120884[index] = 0;
    }
    for (index = 0; index < 8; index++) {
        DAT_0012082c[index] = 0;
    }
    DAT_0012087c = 0;
    lookup_calls = 0;
    lookup_index = -1;
    capture_count = -1;
    for (index = 0; index < 6; index++) {
        capture_queue[index] = 0;
        capture_flags[index] = 0;
    }
    target = 0;
}

static void set_source(int index, int value, int enable, int flag)
{
    DAT_00102108[index] = value;
    DAT_00100002[index] = (unsigned char)enable;
    DAT_00120884[index] = flag;
}

static void fill_five(int base)
{
    int index;

    for (index = 0; index < 5; index++) {
        set_source(index, base + index * 0x100 + 0x2d, 0, 1);
    }
    DAT_00102108[5] = -1;
}

int main(void)
{
    int ok;

    reset();
    DAT_00102108[0] = -1;
    overlay_queue_update();
    put(DAT_0012087c == 0 && DAT_0012082c[0] == 0 && lookup_calls == 0
        && target == 0 ? '0' : 'x');

    reset();
    set_all_keys(0x2d, 0x2d, 0x2d, 0x2d, 0x2d);
    set_source(0, 0x1122332d, 1, 0);
    DAT_00102108[1] = -1;
    overlay_queue_update();
    ok = DAT_00120884[0] == 1 && DAT_0012087c == 0
        && DAT_0012082c[0] == 0 && lookup_calls == 0;
    DAT_00100002[0] = 0;
    overlay_queue_update();
    ok = ok && DAT_00120884[0] == 0 && DAT_0012087c == 1
        && DAT_0012082c[0] == 0x1122332d && DAT_0012082c[1] == 0
        && lookup_calls == 0;
    put(ok ? '1' : 'x');

    reset();
    set_all_keys(0x2d, 0x2d, 0x2d, 0x2d, 0x2d);
    DAT_00102e04[0].value_ptr = &target;
    DAT_00102e04[0].value = 0x11111111;
    DAT_00102e04[0].xor_flag = 0;
    set_source(0, 0x12d, 0, 1);
    set_source(1, 0x22d, 0, 1);
    set_source(2, 0x32d, 0, 1);
    set_source(3, 0x42d, 0, 1);
    set_source(4, 0x52d, 0, 1);
    set_source(5, 0x62d, 0, 1);
    DAT_00102108[6] = -1;
    overlay_queue_update();
    put(lookup_calls == 1 && lookup_index == 8 && capture_count == 4
        && capture_queue[0] == 0x12d && capture_queue[1] == 0x22d
        && capture_queue[2] == 0x32d && capture_queue[3] == 0x42d
        && capture_queue[4] == 0x62d && capture_queue[5] == 0
        && target == 0x11111111
        && DAT_0012087c == 0 && DAT_0012082c[0] == 0 ? '2' : 'x');

    reset();
    set_all_keys(0x2e, 0x2e, 0x2e, 0x2e, 0x2e);
    set_key(4, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d);
    DAT_00102e04[4].value_ptr = &target;
    DAT_00102e04[4].value = 0x000000f0;
    DAT_00102e04[4].xor_flag = 1;
    target = 0x0000000f;
    fill_five(0);
    overlay_queue_update();
    put(target == 0x000000ff && lookup_calls == 1 && lookup_index == 8
        && DAT_0012087c == 0 && DAT_0012082c[0] == 0 ? '3' : 'x');

    reset();
    set_all_keys(0x2e, 0x2e, 0x2e, 0x2e, 0x2e);
    fill_five(0);
    overlay_queue_update();
    put(lookup_calls == 0 && DAT_0012087c == 0 && DAT_0012082c[0] == 0
        && target == 0 ? '4' : 'x');

    reset();
    set_all_keys(0x2d, 0x2d, 0x2d, 0x2d, 0x2d);
    set_source(0, 0x12d, 0, 1);
    set_source(1, 0x22d, 0, 1);
    DAT_00102108[2] = -1;
    overlay_queue_update();
    put(lookup_calls == 0 && DAT_0012087c == 2
        && DAT_0012082c[0] == 0x12d && DAT_0012082c[1] == 0x22d
        && DAT_0012082c[2] == 0 && target == 0 ? '5' : 'x');

    reset();
    set_all_keys(0x2d, 0x2d, 0x2d, 0x2d, 0x2d);
    set_source(0, 0, 0, 1);
    DAT_00102108[1] = -1;
    overlay_queue_update();
    put(DAT_0012087c == 1 && DAT_00120884[0] == 0
        && DAT_0012082c[0] == 0 && DAT_0012082c[1] == 0
        && lookup_calls == 0 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
