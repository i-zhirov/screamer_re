/* The test harness for the rebuilt `choice_load` module.
 *
 * The harness holds the base path, the race context, the 0x460-byte choice
 * record, the format stub, and the three file stubs.  The stubs record their
 * arguments.  The harness runs three paths:
 *
 *     0  the success path built `C:\GAME\config\choice.dat`, opened it with the
 *        mode `rb`, read 0x460 bytes, copied the six fields to the context, and
 *        closed the handle; the record and the remaining context cells keep
 *        their values
 *     1  the failure path wrote the default record: the version fields, the
 *        three 0x6000 limits, the three 1 flags, the two 2 flags, the six -1
 *        index slots, the tag `GRA`, and the six lap and six race records
 *     2  the second success path used another handle and other record fields,
 *        so the six context offsets follow the record fields
 *     D  the exit marker
 *
 * The float checks compare the bit patterns exactly.
 */
#pragma off (check_stack)

#include <stdarg.h>

void choice_load(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct choice_entry {
    int time[5];
    char name[5][4];
    int flag[5];
    char rest[0x14];
};

struct choice_record {
    int field_00;
    int field_04;
    char name[4];
    int field_0c;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    char pad_24[0x18];
    int index[6];
    char pad_54[0x18];
    struct choice_entry lap[6];
    struct choice_entry race[6];
    int field_42c;
    int field_430;
    int field_434;
    int field_438;
    int field_43c;
    char pad_440[4];
    int field_444;
    int field_448;
    int field_44c;
    int field_450;
    int field_454;
    int field_458;
    int field_45c;
};

char DAT_00007edc[0x20];
int DAT_001d2a8;
int *DAT_00002dec;
struct choice_record DAT_00203c4;

static int context[0x20];
static char text[8];
static char *sprintf_buffer;
static char *sprintf_format;
static char *sprintf_base;
static char *got_path;
static char *got_mode;
static void *got_buffer;
static int got_size;
static int got_count;
static int got_stream;
static int got_close;
static int open_result;
static int fill_record;
static int fill_alt;
static int format_calls;
static int open_calls;
static int read_calls;
static int close_calls;

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

static void copy(char *destination, char *source)
{
    int index;

    for (index = 0; source[index] != 0; ++index) {
        destination[index] = source[index];
    }
    destination[index] = 0;
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *base;

    va_start(arguments, format);
    base = va_arg(arguments, char *);
    va_end(arguments);
    sprintf_buffer = buffer;
    sprintf_format = (char *)format;
    sprintf_base = base;
    copy(buffer, "C:\\GAME\\config\\choice.dat");
    format_calls = format_calls + 1;
    return 27;
}

int fopen(char *path, char *mode)
{
    got_path = path;
    got_mode = mode;
    open_calls = open_calls + 1;
    return open_result;
}

int fread(void *buffer, int size, int count, int stream)
{
    int index;

    got_buffer = buffer;
    got_size = size;
    got_count = count;
    got_stream = stream;
    read_calls = read_calls + 1;
    if (fill_record) {
        DAT_00203c4.field_00 = 0x01020304;
        DAT_00203c4.field_04 = 0x11121314;
        copy(DAT_00203c4.name, "XYZ");
        DAT_00203c4.field_0c = 0x21222324;
        DAT_00203c4.field_10 = 0x31323334;
        DAT_00203c4.field_14 = 0x41424344;
        DAT_00203c4.field_18 = 0x51525354;
        DAT_00203c4.field_1c = 0x61626364;
        DAT_00203c4.field_20 = 0x71727374;
        DAT_00203c4.field_42c = 0x81828384;
        DAT_00203c4.field_430 = 0x91929394;
        DAT_00203c4.field_434 = 0xa1a2a3a4;
        DAT_00203c4.field_438 = 0xb1b2b3b4;
        DAT_00203c4.field_43c = 0xc1c2c3c4;
        DAT_00203c4.field_444 = 0xd1d2d3d4;
        DAT_00203c4.field_448 = 0xe1e2e3e4;
        DAT_00203c4.field_44c = 0xf1f2f3f4;
        DAT_00203c4.field_450 = 0x01010102;
        DAT_00203c4.field_454 = 0x01010103;
        DAT_00203c4.field_458 = 0x01010104;
        DAT_00203c4.field_45c = 0x01010105;
        for (index = 0; index < 6; ++index) {
            DAT_00203c4.index[index] = 0x77777777;
        }
        if (fill_alt) {
            DAT_00203c4.field_0c = 0x0a0b0c0d;
            DAT_00203c4.field_14 = 0x1a1b1c1d;
            DAT_00203c4.field_18 = 0x2a2b2c2d;
            DAT_00203c4.field_438 = 0x3a3b3c3d;
            DAT_00203c4.field_43c = 0x4a4b4c4d;
            DAT_00203c4.field_45c = 0x5a5b5c5d;
        }
    }
    return count;
}

int fclose(int stream)
{
    got_close = stream;
    close_calls = close_calls + 1;
    return 0;
}

static void reset(void)
{
    int index;
    int slot;

    for (index = 0; index < 0x20; ++index) {
        context[index] = 0x5a5a5a5a;
    }
    DAT_00002dec = context;
    DAT_001d2a8 = 0x5b5b5b5b;
    copy(DAT_00007edc, "C:\\GAME");

    DAT_00203c4.field_00 = 0x5c5c5c5c;
    DAT_00203c4.field_04 = 0x5c5c5c5c;
    copy(DAT_00203c4.name, "ZZZ");
    DAT_00203c4.field_0c = 0x5c5c5c5c;
    DAT_00203c4.field_10 = 0x5c5c5c5c;
    DAT_00203c4.field_14 = 0x5c5c5c5c;
    DAT_00203c4.field_18 = 0x5c5c5c5c;
    DAT_00203c4.field_1c = 0x5c5c5c5c;
    DAT_00203c4.field_20 = 0x5c5c5c5c;
    for (index = 0; index < 0x18; ++index) {
        DAT_00203c4.pad_24[index] = 0x11;
    }
    for (index = 0; index < 6; ++index) {
        DAT_00203c4.index[index] = 0x5f5f5f5f;
    }
    for (index = 0; index < 0x18; ++index) {
        DAT_00203c4.pad_54[index] = 0x11;
    }
    for (index = 0; index < 6; ++index) {
        for (slot = 0; slot < 5; ++slot) {
            DAT_00203c4.lap[index].time[slot] = 0;
            copy(DAT_00203c4.lap[index].name[slot], "ZZZ");
            DAT_00203c4.lap[index].flag[slot] = 0x5e5e5e5e;
            DAT_00203c4.race[index].time[slot] = 0;
            copy(DAT_00203c4.race[index].name[slot], "ZZZ");
            DAT_00203c4.race[index].flag[slot] = 0x5e5e5e5e;
        }
        for (slot = 0; slot < 0x14; ++slot) {
            DAT_00203c4.lap[index].rest[slot] = 0x11;
            DAT_00203c4.race[index].rest[slot] = 0x11;
        }
    }
    DAT_00203c4.field_42c = 0x5c5c5c5c;
    DAT_00203c4.field_430 = 0x5c5c5c5c;
    DAT_00203c4.field_434 = 0x5c5c5c5c;
    DAT_00203c4.field_438 = 0x5c5c5c5c;
    DAT_00203c4.field_43c = 0x5c5c5c5c;
    for (index = 0; index < 4; ++index) {
        DAT_00203c4.pad_440[index] = 0x11;
    }
    DAT_00203c4.field_444 = 0x5c5c5c5c;
    DAT_00203c4.field_448 = 0x5c5c5c5c;
    DAT_00203c4.field_44c = 0x5c5c5c5c;
    DAT_00203c4.field_450 = 0x5c5c5c5c;
    DAT_00203c4.field_454 = 0x5c5c5c5c;
    DAT_00203c4.field_458 = 0x5c5c5c5c;
    DAT_00203c4.field_45c = 0x5c5c5c5c;

    sprintf_buffer = 0;
    sprintf_format = 0;
    sprintf_base = 0;
    got_path = 0;
    got_mode = 0;
    got_buffer = 0;
    got_size = 0;
    got_count = 0;
    got_stream = 0;
    got_close = 0;
    open_result = 0;
    fill_record = 0;
    fill_alt = 0;
    format_calls = 0;
    open_calls = 0;
    read_calls = 0;
    close_calls = 0;
}

static int entries_ok(struct choice_entry *table, int time_bits)
{
    int index;
    int slot;

    for (index = 0; index < 6; ++index) {
        for (slot = 0; slot < 5; ++slot) {
            if (table[index].time[slot] != time_bits) {
                return 0;
            }
            if (!same(table[index].name[slot], "GRA")) {
                return 0;
            }
            if (table[index].flag[slot] != 0) {
                return 0;
            }
        }
        for (slot = 0; slot < 0x14; ++slot) {
            if (table[index].rest[slot] != 0x11) {
                return 0;
            }
        }
    }
    return 1;
}

static int context_ok(int *changed, int count)
{
    int index;
    int other;
    int skip;

    for (index = 0; index < 0x20; ++index) {
        skip = 0;
        for (other = 0; other < count; ++other) {
            if (changed[other] == index) {
                skip = 1;
            }
        }
        if (!skip && context[index] != 0x5a5a5a5a) {
            return 0;
        }
    }
    return 1;
}

static int defaults_ok(void)
{
    int index;

    if (DAT_00203c4.field_00 != 0 || DAT_00203c4.field_04 != 0) {
        return 0;
    }
    if (DAT_00203c4.field_18 != 0 || DAT_00203c4.field_1c != 0
        || DAT_00203c4.field_20 != 0) {
        return 0;
    }
    if (DAT_00203c4.field_42c != 0 || DAT_00203c4.field_434 != 0) {
        return 0;
    }
    if (DAT_00203c4.field_444 != 0x6000
        || DAT_00203c4.field_448 != 0x6000
        || DAT_00203c4.field_44c != 0x6000) {
        return 0;
    }
    if (DAT_00203c4.field_450 != 1 || DAT_00203c4.field_438 != 1
        || DAT_00203c4.field_43c != 1) {
        return 0;
    }
    if (DAT_00203c4.field_430 != 2 || DAT_00203c4.field_45c != 2) {
        return 0;
    }
    if (DAT_00203c4.field_454 != 0 || DAT_00203c4.field_458 != 0) {
        return 0;
    }
    if (DAT_001d2a8 != 2) {
        return 0;
    }
    if (!same(DAT_00203c4.name, "GRA")) {
        return 0;
    }
    for (index = 0; index < 6; ++index) {
        if (DAT_00203c4.index[index] != -1) {
            return 0;
        }
    }
    if (DAT_00203c4.field_0c != 0x5c5c5c5c
        || DAT_00203c4.field_10 != 0x5c5c5c5c
        || DAT_00203c4.field_14 != 0x5c5c5c5c) {
        return 0;
    }
    for (index = 0; index < 0x18; ++index) {
        if (DAT_00203c4.pad_24[index] != 0x11
            || DAT_00203c4.pad_54[index] != 0x11) {
            return 0;
        }
    }
    for (index = 0; index < 4; ++index) {
        if (DAT_00203c4.pad_440[index] != 0x11) {
            return 0;
        }
    }
    return entries_ok(DAT_00203c4.lap, 0x43340000)
        && entries_ok(DAT_00203c4.race, 0x44610000);
}

int main(void)
{
    int changed[6];

    reset();
    open_result = 0x1234;
    fill_record = 1;
    choice_load();
    changed[0] = 2;
    changed[1] = 6;
    changed[2] = 10;
    changed[3] = 14;
    changed[4] = 18;
    changed[5] = 7;
    put(format_calls == 1
        && same(sprintf_format, "%s\\config\\choice.dat")
        && sprintf_base == DAT_00007edc
        && open_calls == 1
        && same(got_path, "C:\\GAME\\config\\choice.dat")
        && same(got_mode, "rb")
        && read_calls == 1
        && got_buffer == (void *)&DAT_00203c4
        && got_size == 1 && got_count == 0x460 && got_stream == 0x1234
        && close_calls == 1 && got_close == 0x1234
        && context[2] == 0x21222324
        && context[6] == 0x41424344
        && context[10] == 0x51525354
        && context[14] == 0xb1b2b3b4
        && context[18] == 0xc1c2c3c4
        && context[7] == 0x01010105
        && context_ok(changed, 6)
        && DAT_00203c4.field_00 == 0x01020304
        && DAT_00203c4.index[0] == 0x77777777
        && same(DAT_00203c4.name, "XYZ")
        ? '0' : 'x');

    reset();
    open_result = 0;
    choice_load();
    put(defaults_ok()
        && format_calls == 1
        && open_calls == 1
        && read_calls == 0
        && close_calls == 0
        && context_ok(changed, 0)
        && DAT_001d2a8 == 2
        ? '1' : 'x');

    reset();
    open_result = 0x5678;
    fill_record = 1;
    fill_alt = 1;
    choice_load();
    changed[0] = 2;
    changed[1] = 6;
    changed[2] = 10;
    changed[3] = 14;
    changed[4] = 18;
    changed[5] = 7;
    put(format_calls == 1
        && open_calls == 1
        && read_calls == 1 && got_stream == 0x5678
        && close_calls == 1 && got_close == 0x5678
        && context[2] == 0x0a0b0c0d
        && context[6] == 0x1a1b1c1d
        && context[10] == 0x2a2b2c2d
        && context[14] == 0x3a3b3c3d
        && context[18] == 0x4a4b4c4d
        && context[7] == 0x5a5b5c5d
        && context_ok(changed, 6)
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
