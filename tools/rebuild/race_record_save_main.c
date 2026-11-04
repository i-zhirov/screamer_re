/* The test harness for the rebuilt `race_record_save` module.
 *
 * The harness holds the two best-time values, the selected track index, the
 * record gate, the game mode, the base path, the player name, and the two
 * 0x50-byte record tables.  The stubs record the path arguments.  The
 * `sprintf` stub builds the two patterns `%s\replays\temp.rpl` and
 * `%s\replays\REPLAY%d%d.RPL` from character constants, because this minimal
 * image does not load the constant segment, so the module literals are not
 * readable here.
 *
 * The harness runs nine paths:
 *
 *     0  the input bits 0x1020 skip the whole save
 *     1  the gate is not 2: the lap table insert runs and the race table and
 *        the replay stay untouched
 *     2  the lap time is slower than every entry, so the lap table stays
 *        untouched
 *     3  the full path: the lap insert, the race insert, the free replay
 *        number 3, and the delete and the rename
 *     4  the race time is slower than every entry, so the race table and the
 *        replay stay untouched
 *     5  the temporary replay file is missing, so the delete and the rename
 *        do not run
 *     6  the race time equals the first entry: the insert still runs at slot 0
 *     7  the track index 4 selects the fifth record and the first free replay
 *        number
 *     8  the fastest lap time shifts every entry down
 *     D  the exit marker
 */
#pragma off (check_stack)

#include <stdarg.h>

void race_record_save(int index);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned short DAT_00118e1c;
float DAT_0011d26c;
float DAT_0011d268;
int DAT_0011d2b0;
int DAT_0011d260;
char DAT_00007edc[0x40];
char DAT_001203cc[0x40];
unsigned char DAT_00120430[6 * 0x50];
unsigned char DAT_00120610[6 * 0x50];

static char text[8];
static int sprintf_calls;
static int find_calls;
static int find_result;
static int delete_calls;
static int rename_calls;
static char temp_copy[0x60];
static char find_copy[0x60];
static char replay_copy[0x60];
static char delete_copy[0x60];
static char rename_old_copy[0x60];
static char rename_new_copy[0x60];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void copy_text(char *destination, const char *source)
{
    int index;

    index = 0;
    for (;;) {
        destination[index] = source[index];
        if (source[index] == 0) {
            break;
        }
        index = index + 1;
    }
}

static int same(const char *left, const char *right)
{
    int index;

    for (index = 0; left[index] == right[index]; index++) {
        if (left[index] == 0) {
            return 1;
        }
    }
    return 0;
}

static char *put_decimal(char *out, int value)
{
    char digits[12];
    int count;

    count = 0;
    if (value == 0) {
        digits[count] = '0';
        count = count + 1;
    } else {
        while (value > 0) {
            digits[count] = (char)('0' + value % 10);
            count = count + 1;
            value = value / 10;
        }
    }
    while (count > 0) {
        count = count - 1;
        *out = digits[count];
        out = out + 1;
    }
    return out;
}

static char *put_replays(char *out)
{
    *out++ = '\\';
    *out++ = 'r';
    *out++ = 'e';
    *out++ = 'p';
    *out++ = 'l';
    *out++ = 'a';
    *out++ = 'y';
    *out++ = 's';
    *out++ = '\\';
    return out;
}

static char *put_temp_name(char *out)
{
    *out++ = 't';
    *out++ = 'e';
    *out++ = 'm';
    *out++ = 'p';
    *out++ = '.';
    *out++ = 'r';
    *out++ = 'p';
    *out++ = 'l';
    *out = 0;
    return out;
}

static char *put_replay_name(char *out, int index, int counter)
{
    *out++ = 'R';
    *out++ = 'E';
    *out++ = 'P';
    *out++ = 'L';
    *out++ = 'A';
    *out++ = 'Y';
    out = put_decimal(out, index);
    out = put_decimal(out, counter);
    *out++ = '.';
    *out++ = 'R';
    *out++ = 'P';
    *out++ = 'L';
    *out = 0;
    return out;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *out;
    char *base;
    int index;
    int counter;

    (void)format;
    va_start(arguments, format);
    out = buffer;
    base = va_arg(arguments, char *);
    while (*base != 0) {
        *out = *base;
        out = out + 1;
        base = base + 1;
    }
    out = put_replays(out);
    if (sprintf_calls == 0) {
        out = put_temp_name(out);
    } else {
        index = va_arg(arguments, int);
        counter = va_arg(arguments, int);
        out = put_replay_name(out, index, counter);
    }
    va_end(arguments);
    sprintf_calls = sprintf_calls + 1;
    return 0;
}

int find_first(char *path, void *data)
{
    (void)data;
    copy_text(find_copy, path);
    find_calls = find_calls + 1;
    return find_result;
}

void delete_file(char *path)
{
    copy_text(delete_copy, path);
    delete_calls = delete_calls + 1;
}

void rename_file(char *old_path, char *new_path)
{
    copy_text(rename_old_copy, old_path);
    copy_text(rename_new_copy, new_path);
    rename_calls = rename_calls + 1;
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.u = bits;
    *p = v.f;
}

static unsigned int get_bits_float(float value)
{
    union {
        float f;
        unsigned int u;
    } v;

    v.f = value;
    return v.u;
}

static void set_field(unsigned char *table, int index, int slot,
                      unsigned int bits)
{
    set_bits_float((float *)(table + index * 0x50 + slot * 4), bits);
}

static unsigned int field_bits(unsigned char *table, int index, int slot)
{
    return get_bits_float(*(float *)(table + index * 0x50 + slot * 4));
}

static int table_int(unsigned char *table, int index, int offset)
{
    return *(int *)(table + index * 0x50 + offset);
}

static void set_table_int(unsigned char *table, int index, int offset,
                          int value)
{
    *(int *)(table + index * 0x50 + offset) = value;
}

static void set_slot_name(unsigned char *table, int index, int slot,
                          char a, char b, char c)
{
    char *name;

    name = (char *)(table + index * 0x50 + 0x14 + slot * 4);
    name[0] = a;
    name[1] = b;
    name[2] = c;
    name[3] = 0;
}

static int slot_name_is(unsigned char *table, int index, int slot,
                        char a, char b, char c)
{
    char *name;

    name = (char *)(table + index * 0x50 + 0x14 + slot * 4);
    return name[0] == a && name[1] == b && name[2] == c && name[3] == 0;
}

static void reset(void)
{
    int index;

    DAT_00118e1c = 0;
    set_bits_float(&DAT_0011d26c, 0x00000000);
    set_bits_float(&DAT_0011d268, 0x00000000);
    DAT_0011d2b0 = 0;
    DAT_0011d260 = 0;
    DAT_00007edc[0] = 'C';
    DAT_00007edc[1] = ':';
    DAT_00007edc[2] = '\\';
    DAT_00007edc[3] = 'G';
    DAT_00007edc[4] = 'A';
    DAT_00007edc[5] = 'M';
    DAT_00007edc[6] = 'E';
    DAT_00007edc[7] = 0;
    DAT_001203cc[0] = 'A';
    DAT_001203cc[1] = 'B';
    DAT_001203cc[2] = 'C';
    DAT_001203cc[3] = 0;
    for (index = 0; index < 6 * 0x50; index++) {
        DAT_00120430[index] = 0;
        DAT_00120610[index] = 0;
    }
    for (index = 0; index < 6; index++) {
        int slot;

        for (slot = 0; slot < 5; slot++) {
            set_field(DAT_00120430, index, slot, 0x43340000);
            set_field(DAT_00120610, index, slot, 0x44610000);
        }
    }
    sprintf_calls = 0;
    find_calls = 0;
    find_result = 0;
    delete_calls = 0;
    rename_calls = 0;
    temp_copy[0] = 0;
    find_copy[0] = 0;
    replay_copy[0] = 0;
    delete_copy[0] = 0;
    rename_old_copy[0] = 0;
    rename_new_copy[0] = 0;
}

static void expected_temp(char *out)
{
    char *end;

    end = out;
    copy_text(end, DAT_00007edc);
    while (*end != 0) {
        end = end + 1;
    }
    end = put_replays(end);
    put_temp_name(end);
}

static void expected_replay(char *out, int index, int counter)
{
    char *end;

    end = out;
    copy_text(end, DAT_00007edc);
    while (*end != 0) {
        end = end + 1;
    }
    end = put_replays(end);
    put_replay_name(end, index, counter);
}

int main(void)
{
    char expected[0x60];

    reset();
    DAT_00118e1c = 0x1020;
    set_bits_float(&DAT_0011d26c, 0x42c80000);
    race_record_save(0);
    put(sprintf_calls == 0 && find_calls == 0 && delete_calls == 0
        && rename_calls == 0
        && field_bits(DAT_00120430, 0, 0) == 0x43340000 ? '0' : 'x');

    reset();
    DAT_0011d2b0 = 0;
    set_bits_float(&DAT_0011d26c, 0x42c80000);
    race_record_save(0);
    put(field_bits(DAT_00120430, 0, 0) == 0x42c80000
        && table_int(DAT_00120430, 0, 0x28) == 1
        && slot_name_is(DAT_00120430, 0, 0, 'A', 'B', 'C')
        && field_bits(DAT_00120430, 0, 1) == 0x43340000
        && field_bits(DAT_00120610, 0, 0) == 0x44610000
        && sprintf_calls == 0 && find_calls == 0 && delete_calls == 0
        && rename_calls == 0 ? '1' : 'x');

    reset();
    DAT_0011d2b0 = 0;
    set_bits_float(&DAT_0011d26c, 0x42c80000);
    set_field(DAT_00120430, 0, 0, 0x42480000);
    set_field(DAT_00120430, 0, 1, 0x42700000);
    set_field(DAT_00120430, 0, 2, 0x428c0000);
    set_field(DAT_00120430, 0, 3, 0x42a00000);
    set_field(DAT_00120430, 0, 4, 0x42b40000);
    race_record_save(0);
    put(field_bits(DAT_00120430, 0, 0) == 0x42480000
        && field_bits(DAT_00120430, 0, 4) == 0x42b40000
        && table_int(DAT_00120430, 0, 0x28) == 0
        && slot_name_is(DAT_00120430, 0, 0, 0, 0, 0)
        && sprintf_calls == 0 ? '2' : 'x');

    reset();
    DAT_0011d2b0 = 2;
    DAT_0011d260 = 0x20;
    set_bits_float(&DAT_0011d26c, 0x425c0000);
    set_bits_float(&DAT_0011d268, 0x434d0000);
    set_field(DAT_00120430, 0, 0, 0x42480000);
    set_field(DAT_00120430, 0, 1, 0x42700000);
    set_field(DAT_00120430, 0, 2, 0x428c0000);
    set_field(DAT_00120430, 0, 3, 0x42a00000);
    set_field(DAT_00120430, 0, 4, 0x42b40000);
    set_field(DAT_00120610, 0, 0, 0x43480000);
    set_field(DAT_00120610, 0, 1, 0x43520000);
    set_field(DAT_00120610, 0, 2, 0x435c0000);
    set_field(DAT_00120610, 0, 3, 0x43660000);
    set_field(DAT_00120610, 0, 4, 0x43700000);
    set_table_int(DAT_00120610, 0, 0x28, 1);
    set_table_int(DAT_00120610, 0, 0x2c, 2);
    set_slot_name(DAT_00120610, 0, 0, 'X', 'Y', 'Z');
    race_record_save(0);
    {
        char temp_expected[0x60];
        char replay_expected[0x60];

        expected_temp(temp_expected);
        expected_replay(replay_expected, 0, 3);
        put(field_bits(DAT_00120430, 0, 0) == 0x42480000
            && field_bits(DAT_00120430, 0, 1) == 0x425c0000
            && field_bits(DAT_00120430, 0, 2) == 0x42700000
            && field_bits(DAT_00120430, 0, 3) == 0x428c0000
            && field_bits(DAT_00120430, 0, 4) == 0x42a00000
            && table_int(DAT_00120430, 0, 0x2c) == 1
            && slot_name_is(DAT_00120430, 0, 1, 'A', 'B', 'C')
            && field_bits(DAT_00120610, 0, 0) == 0x43480000
            && field_bits(DAT_00120610, 0, 1) == 0x434d0000
            && field_bits(DAT_00120610, 0, 2) == 0x43520000
            && field_bits(DAT_00120610, 0, 3) == 0x435c0000
            && field_bits(DAT_00120610, 0, 4) == 0x43660000
            && table_int(DAT_00120610, 0, 0x28) == 1
            && table_int(DAT_00120610, 0, 0x2c) == 3
            && table_int(DAT_00120610, 0, 0x30) == 2
            && table_int(DAT_00120610, 0, 0x3c) == 0
            && table_int(DAT_00120610, 0, 0x40) == 0x20
            && slot_name_is(DAT_00120610, 0, 0, 'X', 'Y', 'Z')
            && slot_name_is(DAT_00120610, 0, 1, 'A', 'B', 'C')
            && sprintf_calls == 2 && find_calls == 1
            && same(find_copy, temp_expected)
            && delete_calls == 1
            && same(delete_copy, replay_expected)
            && rename_calls == 1
            && same(rename_old_copy, temp_expected)
            && same(rename_new_copy, replay_expected)
            ? '3' : 'x');
    }

    reset();
    DAT_0011d2b0 = 2;
    set_bits_float(&DAT_0011d26c, 0x41200000);
    set_bits_float(&DAT_0011d268, 0x42c80000);
    set_field(DAT_00120430, 0, 0, 0x42480000);
    set_field(DAT_00120430, 0, 1, 0x42700000);
    set_field(DAT_00120430, 0, 2, 0x428c0000);
    set_field(DAT_00120430, 0, 3, 0x42a00000);
    set_field(DAT_00120430, 0, 4, 0x42b40000);
    set_field(DAT_00120610, 0, 0, 0x42480000);
    set_field(DAT_00120610, 0, 1, 0x42700000);
    set_field(DAT_00120610, 0, 2, 0x428c0000);
    set_field(DAT_00120610, 0, 3, 0x42a00000);
    set_field(DAT_00120610, 0, 4, 0x42b40000);
    race_record_save(0);
    put(field_bits(DAT_00120430, 0, 0) == 0x41200000
        && field_bits(DAT_00120430, 0, 1) == 0x42480000
        && field_bits(DAT_00120610, 0, 0) == 0x42480000
        && table_int(DAT_00120610, 0, 0x28) == 0
        && sprintf_calls == 0 && find_calls == 0 ? '4' : 'x');

    reset();
    DAT_0011d2b0 = 2;
    find_result = -1;
    set_bits_float(&DAT_0011d26c, 0x42c80000);
    set_bits_float(&DAT_0011d268, 0x42c80000);
    race_record_save(0);
    put(field_bits(DAT_00120430, 0, 0) == 0x42c80000
        && field_bits(DAT_00120610, 0, 0) == 0x42c80000
        && table_int(DAT_00120610, 0, 0x28) == 0
        && sprintf_calls == 1 && find_calls == 1
        && delete_calls == 0 && rename_calls == 0 ? '5' : 'x');

    reset();
    DAT_0011d2b0 = 2;
    set_bits_float(&DAT_0011d26c, 0x00000000);
    set_bits_float(&DAT_0011d268, 0x43480000);
    set_field(DAT_00120610, 0, 0, 0x43480000);
    set_field(DAT_00120610, 0, 1, 0x43520000);
    set_field(DAT_00120610, 0, 2, 0x435c0000);
    set_field(DAT_00120610, 0, 3, 0x43660000);
    set_field(DAT_00120610, 0, 4, 0x43700000);
    expected_replay(expected, 0, 1);
    race_record_save(0);
    put(field_bits(DAT_00120610, 0, 0) == 0x43480000
        && field_bits(DAT_00120610, 0, 1) == 0x43480000
        && field_bits(DAT_00120610, 0, 4) == 0x43660000
        && table_int(DAT_00120610, 0, 0x28) == 1
        && delete_calls == 1 && same(delete_copy, expected) ? '6' : 'x');

    reset();
    DAT_0011d2b0 = 2;
    set_bits_float(&DAT_0011d26c, 0x41200000);
    set_bits_float(&DAT_0011d268, 0x41200000);
    expected_replay(expected, 4, 1);
    race_record_save(4);
    put(field_bits(DAT_00120430, 4, 0) == 0x41200000
        && field_bits(DAT_00120610, 4, 0) == 0x41200000
        && field_bits(DAT_00120430, 3, 0) == 0x43340000
        && field_bits(DAT_00120610, 3, 0) == 0x44610000
        && table_int(DAT_00120610, 4, 0x28) == 1
        && delete_calls == 1 && same(delete_copy, expected) ? '7' : 'x');

    reset();
    DAT_0011d2b0 = 0;
    set_bits_float(&DAT_0011d26c, 0x41200000);
    set_field(DAT_00120430, 0, 0, 0x42480000);
    set_field(DAT_00120430, 0, 1, 0x42700000);
    set_field(DAT_00120430, 0, 2, 0x428c0000);
    set_field(DAT_00120430, 0, 3, 0x42a00000);
    set_field(DAT_00120430, 0, 4, 0x42b40000);
    race_record_save(0);
    put(field_bits(DAT_00120430, 0, 0) == 0x41200000
        && field_bits(DAT_00120430, 0, 1) == 0x42480000
        && field_bits(DAT_00120430, 0, 4) == 0x42a00000
        && table_int(DAT_00120430, 0, 0x28) == 1
        && slot_name_is(DAT_00120430, 0, 0, 'A', 'B', 'C') ? '8' : 'x');

    put('D');
    quit();
    return 0;
}
