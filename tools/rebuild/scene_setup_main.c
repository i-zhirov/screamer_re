/* The test harness for the rebuilt `scene_setup` module.
 *
 * The harness holds the level prefix, the mode flag word, the player index,
 * the 0x24-byte-stride sound table, the two counts, and a stub for every call
 * target.  The `sprintf` stub formats the text into the buffer, so the two
 * list file calls receive the real names.  The list stub records the six
 * arguments and returns a scripted row count.
 *
 * The harness runs eight paths:
 *
 *     0  the flag bit 3 is set and the player index is 0: the three names are
 *        MAP1i, MAP1A, and MAP1dat; the first list returns 12 and the second
 *        returns 2, so the counts are 12 and 14; the helper and the bank load
 *        run once
 *     1  the flag bit 3 is clear and the player index is 1: the type is MAP1
 *        and the letter is B
 *     2  the mode is 0: the three names build, and no list, helper, or bank
 *        call runs, so the two counts keep their values
 *     3  the first list returns 0: the second table starts at the table base
 *        and the total is the second count
 *     4  the second list returns 0: the second table starts after three
 *        records and the total is the first count
 *     5  the player index is 25: the letter is Z
 *     6  the level prefix is MAP10: every name uses the longer prefix
 *     7  the flag word holds 0x100: bit 3 is clear, so the type has no suffix
 *     D  the exit marker
 *
 * The harness uses only BSS state, because the minimal image does not load the
 * initialized data page.
 */
#pragma off (check_stack)

#include <stdarg.h>

void scene_setup(int mode);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0011d294[0x20];
int DAT_0011d274;
int DAT_0011d2b4;
int DAT_00115224[0x24 * 2];
int DAT_0018d88;
int DAT_0018d84;

static char text[8];

static char sprintf_format[4][0x10];
static char sprintf_result[4][0x40];
static int sprintf_calls;

static char list_type[2][0x20];
static char list_name[2][0x20];
static char list_list[2][0x20];
static void *list_table[2];
static int list_stride[2];
static int list_count[2];
static int list_result[2];
static int list_calls;

static int helper_calls;
static int bank_calls;

static void set_text(char *destination, const char *source)
{
    while (*source != 0) {
        *destination = *source;
        destination = destination + 1;
        source = source + 1;
    }
    *destination = 0;
}

static int same_text(const char *left, const char *right)
{
    while (*left != 0 && *right != 0) {
        if (*left != *right) {
            return 0;
        }
        left = left + 1;
        right = right + 1;
    }
    return *left == *right;
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

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *output;
    const char *cursor;
    char digits[12];
    int count;
    unsigned int value;

    set_text(sprintf_format[sprintf_calls], format);
    output = buffer;
    cursor = format;
    va_start(arguments, format);
    while (*cursor != 0) {
        if (*cursor != '%') {
            *output = *cursor;
            output = output + 1;
            cursor = cursor + 1;
            continue;
        }
        cursor = cursor + 1;
        if (*cursor == 's') {
            const char *word;

            word = va_arg(arguments, const char *);
            while (*word != 0) {
                *output = *word;
                output = output + 1;
                word = word + 1;
            }
            cursor = cursor + 1;
        } else if (*cursor == 'c') {
            *output = (char)va_arg(arguments, int);
            output = output + 1;
            cursor = cursor + 1;
        } else if (*cursor == 'd') {
            int number;

            number = va_arg(arguments, int);
            if (number < 0) {
                *output = '-';
                output = output + 1;
                value = (unsigned int)(-number);
            } else {
                value = (unsigned int)number;
            }
            count = 0;
            do {
                digits[count] = (char)('0' + value % 10);
                count = count + 1;
                value = value / 10;
            } while (value != 0);
            while (count > 0) {
                count = count - 1;
                *output = digits[count];
                output = output + 1;
            }
            cursor = cursor + 1;
        } else if (*cursor == '%') {
            *output = '%';
            output = output + 1;
            cursor = cursor + 1;
        } else {
            *output = '%';
            output = output + 1;
        }
    }
    *output = 0;
    va_end(arguments);
    set_text(sprintf_result[sprintf_calls], buffer);
    sprintf_calls = sprintf_calls + 1;
    return (int)(output - buffer);
}

int list_file_load(char *type, char *name, char *list, void *table,
                   int stride, int count)
{
    set_text(list_type[list_calls], type);
    set_text(list_name[list_calls], name);
    set_text(list_list[list_calls], list);
    list_table[list_calls] = table;
    list_stride[list_calls] = stride;
    list_count[list_calls] = count;
    list_calls = list_calls + 1;
    return list_result[list_calls - 1];
}

void func_24600(void)
{
    helper_calls = helper_calls + 1;
}

void sound_bank_load(void)
{
    bank_calls = bank_calls + 1;
}

static void reset(void)
{
    int index;

    DAT_0011d294[0] = 0;
    DAT_0011d274 = 0;
    DAT_0011d2b4 = 0;
    DAT_0018d88 = 0x11111111;
    DAT_0018d84 = 0x11111111;
    for (index = 0; index < 0x24 * 2; ++index) {
        DAT_00115224[index] = 0;
    }
    sprintf_calls = 0;
    for (index = 0; index < 4; ++index) {
        sprintf_format[index][0] = 0;
        sprintf_result[index][0] = 0;
    }
    list_calls = 0;
    for (index = 0; index < 2; ++index) {
        list_type[index][0] = 0;
        list_name[index][0] = 0;
        list_list[index][0] = 0;
        list_table[index] = 0;
        list_stride[index] = 0;
        list_count[index] = 0;
        list_result[index] = 0;
    }
    helper_calls = 0;
    bank_calls = 0;
}

static int names(const char *base, const char *letter, const char *list)
{
    return sprintf_calls == 3
        && same_text(sprintf_format[0], "%s%s")
        && same_text(sprintf_format[1], "%s%c")
        && same_text(sprintf_format[2], "%sdat")
        && same_text(sprintf_result[0], base)
        && same_text(sprintf_result[1], letter)
        && same_text(sprintf_result[2], list);
}

static int lists(const char *type, const char *list, int first, int second)
{
    return list_calls == 2
        && same_text(list_type[0], "GEN")
        && same_text(list_name[0], "SOUND_FX")
        && same_text(list_list[0], "SFXSBASE")
        && list_table[0] == (void *)DAT_00115224
        && list_stride[0] == 0x24
        && list_count[0] == 0x24
        && same_text(list_type[1], type)
        && same_text(list_name[1], "SOUND_FX")
        && same_text(list_list[1], list)
        && list_stride[1] == 0x24
        && list_count[1] == 0x24
        && list_table[1] == (void *)((char *)DAT_00115224 + first * 0x24)
        && DAT_0018d88 == first
        && DAT_0018d84 == first + second
        && helper_calls == 1
        && bank_calls == 1;
}

static int normal(void)
{
    return names("MAP1i", "MAP1A", "MAP1dat")
        && lists("MAP1i", "MAP1dat", 12, 2);
}

static int no_suffix(void)
{
    return names("MAP1", "MAP1B", "MAP1dat")
        && same_text(list_type[1], "MAP1");
}

static int mode_zero(void)
{
    return names("MAP1i", "MAP1A", "MAP1dat")
        && list_calls == 0
        && helper_calls == 0
        && bank_calls == 0
        && DAT_0018d88 == 0x11111111
        && DAT_0018d84 == 0x11111111;
}

static int zero_first(void)
{
    return names("MAP1i", "MAP1A", "MAP1dat")
        && lists("MAP1i", "MAP1dat", 0, 5);
}

static int zero_second(void)
{
    return names("MAP1i", "MAP1A", "MAP1dat")
        && lists("MAP1i", "MAP1dat", 3, 0);
}

static int letter_z(void)
{
    return names("MAP1", "MAP1Z", "MAP1dat")
        && lists("MAP1", "MAP1dat", 1, 1);
}

static int long_prefix(void)
{
    return names("MAP10", "MAP10A", "MAP10dat")
        && lists("MAP10", "MAP10dat", 1, 1);
}

static int mask_only(void)
{
    return names("MAP1", "MAP1A", "MAP1dat")
        && lists("MAP1", "MAP1dat", 2, 3);
}

int main(void)
{
    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 8;
    DAT_0011d2b4 = 0;
    list_result[0] = 12;
    list_result[1] = 2;
    scene_setup(1);
    put(normal() ? '0' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 0;
    DAT_0011d2b4 = 1;
    list_result[0] = 12;
    list_result[1] = 2;
    scene_setup(1);
    put(no_suffix() ? '1' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 8;
    DAT_0011d2b4 = 0;
    scene_setup(0);
    put(mode_zero() ? '2' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 8;
    DAT_0011d2b4 = 0;
    list_result[0] = 0;
    list_result[1] = 5;
    scene_setup(1);
    put(zero_first() ? '3' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 8;
    DAT_0011d2b4 = 0;
    list_result[0] = 3;
    list_result[1] = 0;
    scene_setup(1);
    put(zero_second() ? '4' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 0;
    DAT_0011d2b4 = 25;
    list_result[0] = 1;
    list_result[1] = 1;
    scene_setup(1);
    put(letter_z() ? '5' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP10");
    DAT_0011d274 = 0;
    DAT_0011d2b4 = 0;
    list_result[0] = 1;
    list_result[1] = 1;
    scene_setup(1);
    put(long_prefix() ? '6' : 'x');

    reset();
    set_text(DAT_0011d294, "MAP1");
    DAT_0011d274 = 0x100;
    DAT_0011d2b4 = 0;
    list_result[0] = 2;
    list_result[1] = 3;
    scene_setup(1);
    put(mask_only() ? '7' : 'x');

    put('D');
    quit();
    return 0;
}
