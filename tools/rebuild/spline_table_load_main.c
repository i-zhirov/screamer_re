/* The test harness for the rebuilt `spline_table_load` module.
 *
 * The harness holds the level prefix, the game mode, the five member name
 * buffers, the five spline table pointers, and the five counts.  The stubs
 * record their arguments and their call counts.  The `sprintf` stub copies the
 * prefix and appends the text after `%s` of the format.  The `spline_load` stub
 * returns a chosen count and writes a chosen table pointer through the third
 * argument.
 *
 * The harness runs four paths:
 *
 *     0  the game mode 0x20: the fifth member name uses the suffix CNG
 *     1  the game mode 0x40: the fifth member name uses the suffix SLA
 *     2  the game mode 0x03: the fifth member name uses the suffix OST
 *     3  the game mode 0x50: the fifth member name uses the suffix OST
 *     D  the exit marker
 *
 * DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero.  The
 * harness builds every text in writable data at run time, and the module build
 * defines `SPLINE_TABLE_LOAD_HARNESS` for the same reason.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal.
 */
#pragma off (check_stack)

#include <stdarg.h>

int spline_table_load(char *level);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_0011d294[0x20];
int DAT_0011d260;

char DAT_0010f8d8[0x20];
void *DAT_0010f8e4;
int DAT_0010f8e8;

char DAT_0010f168[0x20];
void *DAT_0010f174;
int DAT_0010f178;

char DAT_0010f8f4[0x20];
void *DAT_0010f90c;
int DAT_0010f914;

char DAT_0010f900[0x20];
void *DAT_0010f910;
int DAT_0010f918;

char DAT_0010d6e8[0x20];
void *DAT_0010d6f4;
int DAT_0010d6fc;

static char text[8];

static int sprintf_calls;
static char *sprintf_buffer[8];
static char sprintf_format_copy[8][0x10];
static char sprintf_result_copy[8][0x20];
static unsigned int sprintf_arg[8];

static int spline_calls;
static unsigned int spline_archive[8];
static char spline_archive_copy[8][0x10];
static char spline_name_copy[8][0x20];
static void **spline_table[8];
static void *spline_value[8];
static int spline_count[8];

static char level_text[0x10];
static int path_counts[5];

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

    fill(DAT_0011d294, 'M', 'A', 'P', '1', 0, 0, 0, 0);
    DAT_0011d260 = 0;
    DAT_0010f8d8[0] = 0;
    DAT_0010f168[0] = 0;
    DAT_0010f8f4[0] = 0;
    DAT_0010f900[0] = 0;
    DAT_0010d6e8[0] = 0;
    DAT_0010f8e4 = 0;
    DAT_0010f174 = 0;
    DAT_0010f90c = 0;
    DAT_0010f910 = 0;
    DAT_0010d6f4 = 0;
    DAT_0010f8e8 = 0;
    DAT_0010f178 = 0;
    DAT_0010f914 = 0;
    DAT_0010f918 = 0;
    DAT_0010d6fc = 0;
    fill(level_text, 'L', 'E', 'V', '1', 0, 0, 0, 0);
    sprintf_calls = 0;
    for (index = 0; index < 8; ++index) {
        sprintf_buffer[index] = 0;
        sprintf_format_copy[index][0] = 0;
        sprintf_result_copy[index][0] = 0;
        sprintf_arg[index] = 0;
    }
    spline_calls = 0;
    for (index = 0; index < 8; ++index) {
        spline_archive[index] = 0;
        spline_archive_copy[index][0] = 0;
        spline_name_copy[index][0] = 0;
        spline_table[index] = 0;
        spline_value[index] = 0;
        spline_count[index] = 0;
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
    append(buffer, (char *)format + 2);
    if (index < 8) {
        sprintf_buffer[index] = buffer;
        copy(sprintf_format_copy[index], (char *)format);
        sprintf_arg[index] = (unsigned int)first;
        copy(sprintf_result_copy[index], buffer);
    }
    sprintf_calls = index + 1;
    return 0;
}

int spline_load(char *archive, char *name, void **table)
{
    int index;

    index = spline_calls;
    if (index < 8) {
        spline_archive[index] = (unsigned int)archive;
        copy(spline_archive_copy[index], archive);
        copy(spline_name_copy[index], name);
        spline_table[index] = table;
        *table = spline_value[index];
    }
    spline_calls = index + 1;
    if (index < 8) {
        return spline_count[index];
    }
    return 0;
}

static int check_path(char *mode_suffix)
{
    char suffix[5][0x10];
    char format[5][0x10];
    char name[5][0x20];
    void **table[5];
    int *count[5];
    int index;

    fill(suffix[0], 'o', 'p', 'p', 0, 0, 0, 0, 0);
    fill(suffix[1], 'a', 'n', 'm', 0, 0, 0, 0, 0);
    fill(suffix[2], 'c', 'a', 'm', 0, 0, 0, 0, 0);
    fill(suffix[3], 'r', 'p', 'l', 0, 0, 0, 0, 0);
    copy(suffix[4], mode_suffix);

    for (index = 0; index < 5; ++index) {
        format[index][0] = '%';
        format[index][1] = 's';
        format[index][2] = 0;
        append(format[index], suffix[index]);
        copy(name[index], DAT_0011d294);
        append(name[index], suffix[index]);
    }

    table[0] = &DAT_0010f8e4;
    table[1] = &DAT_0010f174;
    table[2] = &DAT_0010f90c;
    table[3] = &DAT_0010f910;
    table[4] = &DAT_0010d6f4;
    count[0] = &DAT_0010f8e8;
    count[1] = &DAT_0010f178;
    count[2] = &DAT_0010f914;
    count[3] = &DAT_0010f918;
    count[4] = &DAT_0010d6fc;

    if (sprintf_calls != 5 || spline_calls != 5) {
        return 0;
    }
    for (index = 0; index < 5; ++index) {
        if (!same(sprintf_format_copy[index], format[index])) {
            return 0;
        }
        if (sprintf_arg[index] != (unsigned int)DAT_0011d294) {
            return 0;
        }
        if (!same(sprintf_result_copy[index], name[index])) {
            return 0;
        }
        if (spline_archive[index] != (unsigned int)level_text) {
            return 0;
        }
        if (!same(spline_archive_copy[index], level_text)) {
            return 0;
        }
        if (!same(spline_name_copy[index], name[index])) {
            return 0;
        }
        if (spline_table[index] != table[index]) {
            return 0;
        }
        if (*table[index] != spline_value[index]) {
            return 0;
        }
        if (*count[index] != spline_count[index]) {
            return 0;
        }
        if (spline_count[index] != path_counts[index]) {
            return 0;
        }
    }
    return 1;
}

static void set_values(void)
{
    int index;

    for (index = 0; index < 5; ++index) {
        spline_value[index] = (void *)(0x1000 + index * 0x10);
        spline_count[index] = 2 + index;
        path_counts[index] = 2 + index;
    }
}

int main(void)
{
    char cng[0x10];
    char sla[0x10];
    char ost[0x10];

    fill(cng, 'C', 'N', 'G', 0, 0, 0, 0, 0);
    fill(sla, 'S', 'L', 'A', 0, 0, 0, 0, 0);
    fill(ost, 'O', 'S', 'T', 0, 0, 0, 0, 0);

    reset();
    set_values();
    DAT_0011d260 = 0x20;
    spline_table_load(level_text);
    put(check_path(cng) ? '0' : 'x');

    reset();
    set_values();
    DAT_0011d260 = 0x40;
    spline_table_load(level_text);
    put(check_path(sla) ? '1' : 'x');

    reset();
    set_values();
    DAT_0011d260 = 0x03;
    spline_table_load(level_text);
    put(check_path(ost) ? '2' : 'x');

    reset();
    set_values();
    DAT_0011d260 = 0x50;
    spline_table_load(level_text);
    put(check_path(ost) ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
