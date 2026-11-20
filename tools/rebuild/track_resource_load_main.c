/* The test harness for the rebuilt `track_resource_load` module.
 *
 * The harness holds the level name, the game mode byte, the two tables, and
 * the two counts.  The `track_marker_load` stub records the two name
 * arguments, the table, and the call count, writes one byte to the table, and
 * returns a chosen count.  The `sprintf` stub copies the first variable
 * argument and appends `sla`, so the second load receives the SLA member name.
 *
 * The harness runs four paths:
 *
 *     0  the game mode 0x00: the function calls the loader once and leaves the
 *        SLA count unchanged
 *     1  the game mode 0x40: the function calls the loader twice and stores the
 *        second count at DAT_00007ed4
 *     2  the game mode 0x50: bit 0x40 is set, so the second load also runs
 *     3  the game mode 0x40 with a zero first count: the second load still runs
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the `sprintf`
 * stub ignores the format pointer, so the literal in the module is not read.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

void track_resource_load(char *level);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0001d260;
char DAT_0001d294[0x20];
char DAT_00007250[0x100];
char DAT_00007890[0x100];
int DAT_00007ed0;
int DAT_00007ed4;

static char text[8];

static char level_text[0x10];
static char name_text[0x20];
static char sla_suffix[0x10];
static char sla_name[0x20];

static int sprintf_calls;
static unsigned int sprintf_arg[4];
static char sprintf_result_copy[4][0x20];

static int load_calls;
static unsigned int load_archive[4];
static char load_name_copy[4][0x20];
static void *load_table[4];
static int load_count[4];

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

static void reset(void)
{
    int index;

    DAT_0001d260 = 0;
    fill(DAT_0001d294, 'M', 'A', 'P', '1', 'I', 0, 0, 0);
    DAT_00007250[0] = 0;
    DAT_00007890[0] = 0;
    DAT_00007ed0 = 0x1234;
    DAT_00007ed4 = 0x5678;
    fill(level_text, 'L', 'E', 'V', '1', 0, 0, 0, 0);
    fill(name_text, 'M', 'A', 'P', '1', 'I', 0, 0, 0);
    fill(sla_suffix, 's', 'l', 'a', 0, 0, 0, 0, 0);
    copy(sla_name, name_text);
    append(sla_name, sla_suffix);
    sprintf_calls = 0;
    load_calls = 0;
    for (index = 0; index < 4; ++index) {
        sprintf_arg[index] = 0;
        sprintf_result_copy[index][0] = 0;
        load_archive[index] = 0;
        load_name_copy[index][0] = 0;
        load_table[index] = 0;
        load_count[index] = 0;
    }
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list arguments;
    char *first;
    int index;

    index = sprintf_calls;
    va_start(arguments, format);
    first = va_arg(arguments, char *);
    va_end(arguments);
    copy(buffer, first);
    append(buffer, sla_suffix);
    if (index < 4) {
        sprintf_arg[index] = (unsigned int)first;
        copy(sprintf_result_copy[index], buffer);
    }
    sprintf_calls = index + 1;
    return 0;
}

int track_marker_load(char *archive, char *name, void *table)
{
    int index;

    index = load_calls;
    if (index < 4) {
        load_archive[index] = (unsigned int)archive;
        copy(load_name_copy[index], name);
        load_table[index] = table;
        ((unsigned char *)table)[0] = (unsigned char)(0xa0 + index);
    }
    load_calls = index + 1;
    if (index < 4) {
        return load_count[index];
    }
    return 0;
}

static int check_single(void)
{
    track_resource_load(level_text);
    return load_calls == 1
        && load_archive[0] == (unsigned int)level_text
        && same(load_name_copy[0], name_text)
        && load_table[0] == (void *)DAT_00007250
        && DAT_00007250[0] == 0xa0
        && DAT_00007ed0 == 3
        && DAT_00007ed4 == 0x5678
        && sprintf_calls == 0;
}

static int check_sla(void)
{
    track_resource_load(level_text);
    return load_calls == 2
        && load_archive[0] == (unsigned int)level_text
        && same(load_name_copy[0], name_text)
        && load_table[0] == (void *)DAT_00007250
        && DAT_00007ed0 == 2
        && sprintf_calls == 1
        && sprintf_arg[0] == (unsigned int)DAT_0001d294
        && same(sprintf_result_copy[0], sla_name)
        && load_archive[1] == (unsigned int)level_text
        && same(load_name_copy[1], sla_name)
        && load_table[1] == (void *)DAT_00007890
        && DAT_00007890[0] == 0xa1
        && DAT_00007ed4 == 5;
}

static int check_zero(void)
{
    track_resource_load(level_text);
    return load_calls == 2
        && DAT_00007ed0 == 0
        && DAT_00007ed4 == 1
        && sprintf_calls == 1
        && same(load_name_copy[1], sla_name);
}

int main(void)
{
    reset();
    DAT_0001d260 = 0x00;
    load_count[0] = 3;
    put(check_single() ? '0' : 'a');

    reset();
    DAT_0001d260 = 0x40;
    load_count[0] = 2;
    load_count[1] = 5;
    put(check_sla() ? '1' : 'b');

    reset();
    DAT_0001d260 = 0x50;
    load_count[0] = 2;
    load_count[1] = 5;
    put(check_sla() ? '2' : 'c');

    reset();
    DAT_0001d260 = 0x40;
    load_count[0] = 0;
    load_count[1] = 1;
    put(check_zero() ? '3' : 'd');

    put('D');
    quit();
    return 0;
}
