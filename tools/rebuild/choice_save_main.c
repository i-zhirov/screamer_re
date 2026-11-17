/* The test harness for the rebuilt `choice_save` module.
 *
 * The harness holds the base path, the race context, the 0x460-byte choice
 * record, the five option globals, and the four file stubs.  The stubs record
 * their arguments.  The harness runs three paths:
 *
 *     0  the success path built `C:\GAME\config\choice.dat`, opened it with the
 *        mode `wb`, copied the six context fields and the five option globals
 *        into the record, wrote 0x460 bytes, and closed the handle; the other
 *        record fields keep their values
 *     1  the failure path opened nothing, so the record keeps every value and
 *        the write and close stubs do not run
 *     2  the second success path used another handle and other values, so the
 *        six context offsets and the five global fields follow them
 *     D  the exit marker
 */
#pragma off (check_stack)

#include <stdarg.h>

void choice_save(void);
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
int DAT_001d2a4;
int DAT_001d2a8;
int DAT_001d2bc;
int DAT_001d2c0;
int DAT_001d2c4;
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
static int format_calls;
static int open_calls;
static int write_calls;
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

int fwrite(void *buffer, int size, int count, int stream)
{
    got_buffer = buffer;
    got_size = size;
    got_count = count;
    got_stream = stream;
    write_calls = write_calls + 1;
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
    DAT_001d2a4 = 0x5b5b5b5b;
    DAT_001d2a8 = 0x5c5c5c5c;
    DAT_001d2bc = 0x5d5d5d5d;
    DAT_001d2c0 = 0x5e5e5e5e;
    DAT_001d2c4 = 0x5f5f5f5f;
    copy(DAT_00007edc, "C:\\GAME");

    DAT_00203c4.field_00 = 0x10000000;
    DAT_00203c4.field_04 = 0x10000001;
    copy(DAT_00203c4.name, "ZZZ");
    DAT_00203c4.field_0c = 0x1000000c;
    DAT_00203c4.field_10 = 0x10000010;
    DAT_00203c4.field_14 = 0x10000014;
    DAT_00203c4.field_18 = 0x10000018;
    DAT_00203c4.field_1c = 0x1000001c;
    DAT_00203c4.field_20 = 0x10000020;
    for (index = 0; index < 0x18; ++index) {
        DAT_00203c4.pad_24[index] = 0x11;
    }
    for (index = 0; index < 6; ++index) {
        DAT_00203c4.index[index] = 0x10101010;
    }
    for (index = 0; index < 0x18; ++index) {
        DAT_00203c4.pad_54[index] = 0x12;
    }
    for (index = 0; index < 6; ++index) {
        for (slot = 0; slot < 5; ++slot) {
            DAT_00203c4.lap[index].time[slot] = 0x10101020;
            copy(DAT_00203c4.lap[index].name[slot], "ZZZ");
            DAT_00203c4.lap[index].flag[slot] = 0x10101021;
            DAT_00203c4.race[index].time[slot] = 0x10101022;
            copy(DAT_00203c4.race[index].name[slot], "ZZZ");
            DAT_00203c4.race[index].flag[slot] = 0x10101023;
        }
        for (slot = 0; slot < 0x14; ++slot) {
            DAT_00203c4.lap[index].rest[slot] = 0x13;
            DAT_00203c4.race[index].rest[slot] = 0x14;
        }
    }
    DAT_00203c4.field_42c = 0x1000002c;
    DAT_00203c4.field_430 = 0x10000030;
    DAT_00203c4.field_434 = 0x10000034;
    DAT_00203c4.field_438 = 0x10000038;
    DAT_00203c4.field_43c = 0x1000003c;
    for (index = 0; index < 4; ++index) {
        DAT_00203c4.pad_440[index] = 0x15;
    }
    DAT_00203c4.field_444 = 0x10000044;
    DAT_00203c4.field_448 = 0x10000048;
    DAT_00203c4.field_44c = 0x1000004c;
    DAT_00203c4.field_450 = 0x10000050;
    DAT_00203c4.field_454 = 0x10000054;
    DAT_00203c4.field_458 = 0x10000058;
    DAT_00203c4.field_45c = 0x1000005c;

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
    format_calls = 0;
    open_calls = 0;
    write_calls = 0;
    close_calls = 0;
}

static int untouched_ok(void)
{
    int index;
    int slot;

    if (DAT_00203c4.field_00 != 0x10000000
        || DAT_00203c4.field_04 != 0x10000001
        || DAT_00203c4.field_18 != 0x10000018
        || DAT_00203c4.field_1c != 0x1000001c
        || DAT_00203c4.field_20 != 0x10000020
        || DAT_00203c4.field_42c != 0x1000002c
        || DAT_00203c4.field_434 != 0x10000034
        || DAT_00203c4.field_454 != 0x10000054
        || DAT_00203c4.field_458 != 0x10000058) {
        return 0;
    }
    if (!same(DAT_00203c4.name, "ZZZ")) {
        return 0;
    }
    for (index = 0; index < 6; ++index) {
        if (DAT_00203c4.index[index] != 0x10101010) {
            return 0;
        }
    }
    for (index = 0; index < 0x18; ++index) {
        if (DAT_00203c4.pad_24[index] != 0x11
            || DAT_00203c4.pad_54[index] != 0x12) {
            return 0;
        }
    }
    for (index = 0; index < 4; ++index) {
        if (DAT_00203c4.pad_440[index] != 0x15) {
            return 0;
        }
    }
    for (index = 0; index < 6; ++index) {
        for (slot = 0; slot < 5; ++slot) {
            if (DAT_00203c4.lap[index].time[slot] != 0x10101020
                || DAT_00203c4.lap[index].flag[slot] != 0x10101021
                || DAT_00203c4.race[index].time[slot] != 0x10101022
                || DAT_00203c4.race[index].flag[slot] != 0x10101023) {
                return 0;
            }
            if (!same(DAT_00203c4.lap[index].name[slot], "ZZZ")
                || !same(DAT_00203c4.race[index].name[slot], "ZZZ")) {
                return 0;
            }
        }
        for (slot = 0; slot < 0x14; ++slot) {
            if (DAT_00203c4.lap[index].rest[slot] != 0x13
                || DAT_00203c4.race[index].rest[slot] != 0x14) {
                return 0;
            }
        }
    }
    return 1;
}

static int file_ok(int handle)
{
    return format_calls == 1
        && same(sprintf_format, "%s\\config\\choice.dat")
        && sprintf_base == DAT_00007edc
        && open_calls == 1
        && same(got_path, "C:\\GAME\\config\\choice.dat")
        && same(got_mode, "wb")
        && write_calls == 1
        && got_buffer == (void *)&DAT_00203c4
        && got_size == 1 && got_count == 0x460 && got_stream == handle
        && close_calls == 1 && got_close == handle;
}

int main(void)
{
    reset();
    context[2] = 0x21222324;
    context[6] = 0x31323334;
    context[10] = 0x41424344;
    context[14] = 0x51525354;
    context[18] = 0x61626364;
    context[7] = 0x71727374;
    DAT_001d2a4 = 0x81828384;
    DAT_001d2a8 = 0x91929394;
    DAT_001d2bc = 0xa1a2a3a4;
    DAT_001d2c0 = 0xb1b2b3b4;
    DAT_001d2c4 = 0xc1c2c3c4;
    open_result = 0x1234;
    choice_save();
    put(file_ok(0x1234)
        && DAT_00203c4.field_0c == 0x21222324
        && DAT_00203c4.field_10 == 0x31323334
        && DAT_00203c4.field_14 == 0x41424344
        && DAT_00203c4.field_438 == 0x51525354
        && DAT_00203c4.field_43c == 0x61626364
        && DAT_00203c4.field_45c == 0x71727374
        && DAT_00203c4.field_450 == 0x81828384
        && DAT_00203c4.field_430 == 0x91929394
        && DAT_00203c4.field_448 == 0xb1b2b3b4
        && DAT_00203c4.field_444 == 0xa1a2a3a4
        && DAT_00203c4.field_44c == 0xc1c2c3c4
        && untouched_ok()
        ? '0' : 'x');

    reset();
    open_result = 0;
    choice_save();
    put(format_calls == 1 && open_calls == 1
        && write_calls == 0 && close_calls == 0
        && DAT_00203c4.field_0c == 0x1000000c
        && DAT_00203c4.field_10 == 0x10000010
        && DAT_00203c4.field_14 == 0x10000014
        && DAT_00203c4.field_438 == 0x10000038
        && DAT_00203c4.field_43c == 0x1000003c
        && DAT_00203c4.field_45c == 0x1000005c
        && DAT_00203c4.field_450 == 0x10000050
        && DAT_00203c4.field_430 == 0x10000030
        && DAT_00203c4.field_448 == 0x10000048
        && DAT_00203c4.field_444 == 0x10000044
        && DAT_00203c4.field_44c == 0x1000004c
        && untouched_ok()
        ? '1' : 'x');

    reset();
    context[2] = 0x0a0b0c0d;
    context[6] = 0x1a1b1c1d;
    context[10] = 0x2a2b2c2d;
    context[14] = 0x3a3b3c3d;
    context[18] = 0x4a4b4c4d;
    context[7] = 0x5a5b5c5d;
    DAT_001d2a4 = 0x6a6b6c6d;
    DAT_001d2a8 = 0x7a7b7c7d;
    DAT_001d2bc = 0x8a8b8c8d;
    DAT_001d2c0 = 0x9a9b9c9d;
    DAT_001d2c4 = 0xaaabacad;
    open_result = 0x5678;
    choice_save();
    put(file_ok(0x5678)
        && DAT_00203c4.field_0c == 0x0a0b0c0d
        && DAT_00203c4.field_10 == 0x1a1b1c1d
        && DAT_00203c4.field_14 == 0x2a2b2c2d
        && DAT_00203c4.field_438 == 0x3a3b3c3d
        && DAT_00203c4.field_43c == 0x4a4b4c4d
        && DAT_00203c4.field_45c == 0x5a5b5c5d
        && DAT_00203c4.field_450 == 0x6a6b6c6d
        && DAT_00203c4.field_430 == 0x7a7b7c7d
        && DAT_00203c4.field_448 == 0x9a9b9c9d
        && DAT_00203c4.field_444 == 0x8a8b8c8d
        && DAT_00203c4.field_44c == 0xaaabacad
        && untouched_ok()
        ? '2' : 'x');

    put('D');
    quit();
    return 0;
}
