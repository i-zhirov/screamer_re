/* The test harness for the rebuilt `player_car_init` module.
 *
 * The harness holds the car record table, the selected index, the player
 * object, and the stubs.  The stubs record their arguments and their call
 * counts.  The sprintf stub builds the two cockpit names.
 *
 * The program runs six paths:
 *
 *     1  the normal record, the letter A, and the two object loads
 *     2  the animation flag set, so the letter B
 *     3  the first object load fails
 *     4  the second object load fails
 *     0  the zero load flag
 *     D  the done marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, so the test needs no
 * initialized data page.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

#include <stdarg.h>

int player_car_init(void *car, int load);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0000d25c[0x100];
int DAT_0011d2a0;

static char text[8];
static char player[0x4b0];
static char auto_text[0x10];
static char shadow_text[0x10];
static char cock_text[0x10];
static char name_a[0x10];
static char name_b[0x10];
static char name_h[0x10];

static int format_calls;
static unsigned int format_first;
static unsigned int format_second;
static char *name_first;
static char *name_second;
static int suffix_first;
static int suffix_second;

static int cockpit_calls;
static char cockpit_name[0x20];
static void **cockpit_out;

static int list_calls;
static char *list_type;
static char *list_name;
static char list_list[0x20];
static void *list_table;
static int list_stride;
static int list_count;

static int object_calls;
static void *object_table[2];
static char object_name[2][0x20];
static int *object_slot[2];
static int object_result[2];

static void put(char a)
{
    text[0] = a;
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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

static void append(char *destination, char *source)
{
    int index;

    for (index = 0; destination[index] != 0; ++index) {
    }
    copy(destination + index, source);
}

static void fill(char *target, char a, char b, char c, char d,
                 char e, char f, char g, char h)
{
    target[0] = a;
    target[1] = b;
    target[2] = c;
    target[3] = d;
    target[4] = e;
    target[5] = f;
    target[6] = g;
    target[7] = h;
    target[8] = 0;
}

static void init_text(void)
{
    fill(auto_text, 'A', 'U', 'T', 'O', '0', '0', 0, 0);
    fill(shadow_text, 'S', 'H', 'A', 'D', 'O', 'W', 0, 0);
    fill(cock_text, 'C', 'O', 'C', 'K', 0, 0, 0, 0);
    fill(name_a, 'C', 'O', 'C', 'K', '0', '0', 'A', 0);
    fill(name_b, 'C', 'O', 'C', 'K', '0', '0', 'B', 0);
    fill(name_h, 'C', 'O', 'C', 'K', '0', '0', 'H', 0);
}

int sprintf(char *buffer, char *format, ...)
{
    va_list arguments;
    char *source;
    int suffix;
    int length;

    va_start(arguments, format);
    source = va_arg(arguments, char *);
    suffix = va_arg(arguments, int);
    va_end(arguments);
    format_calls = format_calls + 1;
    if (format_calls == 1) {
        format_first = (unsigned int)format;
        name_first = source;
        suffix_first = suffix;
    } else {
        format_second = (unsigned int)format;
        name_second = source;
        suffix_second = suffix;
    }
    copy(buffer, cock_text);
    append(buffer, source);
    for (length = 0; buffer[length] != 0; ++length) {
    }
    buffer[length] = (char)suffix;
    buffer[length + 1] = 0;
    return 0;
}

void cockpit_resource_load(char *name, void **out)
{
    cockpit_calls = cockpit_calls + 1;
    copy(cockpit_name, name);
    cockpit_out = out;
    if (out != 0) {
        *out = (void *)0x12345678;
    }
}

int list_file_load(char *type, char *name, char *list, void *table,
                   int stride, int count)
{
    list_calls = list_calls + 1;
    list_type = type;
    list_name = name;
    copy(list_list, list);
    list_table = table;
    list_stride = stride;
    list_count = count;
    return 0;
}

int object_load_named(void *table, char *name, int *slot)
{
    int index;

    index = object_calls;
    object_calls = object_calls + 1;
    object_table[index] = table;
    object_slot[index] = slot;
    if (name == (char *)0x57f) {
        copy(object_name[index], shadow_text);
    } else {
        copy(object_name[index], name);
    }
    if (slot != 0) {
        *slot = 0x1111 + index;
    }
    return object_result[index];
}

static void fill_record(int animation)
{
    char *record;
    int index;

    for (index = 0; index < 0x100; ++index) {
        DAT_0000d25c[index] = 0;
    }
    DAT_0011d2a0 = 1;
    record = DAT_0000d25c + 0x40;
    copy(record, auto_text);
    record[0xc] = (char)animation;
    record[0xd] = (char)(animation >> 8);
    record[0xe] = (char)(animation >> 16);
    record[0xf] = (char)(animation >> 24);
}

static void prepare(int first_result, int second_result)
{
    int index;

    format_calls = 0;
    format_first = 0;
    format_second = 0;
    name_first = 0;
    name_second = 0;
    suffix_first = 0;
    suffix_second = 0;
    cockpit_calls = 0;
    cockpit_name[0] = 0;
    cockpit_out = 0;
    list_calls = 0;
    list_type = 0;
    list_name = 0;
    list_list[0] = 0;
    list_table = 0;
    list_stride = 0;
    list_count = 0;
    object_calls = 0;
    object_table[0] = 0;
    object_table[1] = 0;
    object_name[0][0] = 0;
    object_name[1][0] = 0;
    object_slot[0] = 0;
    object_slot[1] = 0;
    object_result[0] = first_result;
    object_result[1] = second_result;
    for (index = 0; index < 0x4b0; ++index) {
        player[index] = 0;
    }
}

static int common_result(int animation, int letter)
{
    return format_calls == 2
        && format_first == 0x564
        && format_second == 0x56d
        && suffix_first == letter
        && suffix_second == 'H'
        && name_first == player + 0x474
        && name_second == player + 0x474
        && cockpit_calls == 1
        && same(cockpit_name, letter == 'A' ? name_a : name_b)
        && cockpit_out == (void **)0x18f0c
        && list_calls == 1
        && list_type == (char *)0x57b
        && list_name == (char *)0x576
        && same(list_list, name_h)
        && list_table == (void *)0x18f1c
        && list_stride == 0x74
        && list_count == 0x74
        && *(int *)(player + 0x47c) == animation;
}

int main(void)
{
    int result;

    init_text();

    fill_record(0);
    prepare(0, 0);
    result = player_car_init(player, 1);
    put(result == 0
        && same(player + 0x470, auto_text)
        && common_result(0, 'A')
        && object_calls == 2
        && object_table[0] == (void *)0x6530
        && object_table[1] == (void *)0x6530
        && same(object_name[0], shadow_text)
        && same(object_name[1], auto_text)
        && object_slot[0] == (int *)(player + 0x468)
        && object_slot[1] == (int *)(player + 0x460)
        && *(int *)(player + 0x468) == 0x1111
        && *(int *)(player + 0x460) == 0x1112 ? '1' : 'x');

    fill_record(0x8000);
    prepare(0, 0);
    result = player_car_init(player, 1);
    put(result == 0
        && common_result(0x8000, 'B')
        && object_calls == 2 ? '2' : 'x');

    fill_record(0);
    prepare(5, 0);
    result = player_car_init(player, 1);
    put(result == 5 && object_calls == 1 ? '3' : 'x');

    fill_record(0);
    prepare(0, 7);
    result = player_car_init(player, 1);
    put(result == 7 && object_calls == 2 ? '4' : 'x');

    fill_record(0);
    prepare(0, 0);
    result = player_car_init(player, 0);
    put(result == 0
        && format_calls == 0
        && cockpit_calls == 0
        && list_calls == 0
        && object_calls == 0 ? '0' : 'x');

    put('D');
    quit();
    return 0;
}
