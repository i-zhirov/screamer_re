/* The test harness for the rebuilt `sound_effect_list_init` module.
 *
 * The harness holds the state, the mode, the record index, the three output
 * cells, the arena pointer, the 0x24-byte sample table, and a stub for every
 * call target.  The `memory_available` stub returns a scripted value and counts
 * the calls.  The `list_file_load` stub records the six arguments and returns a
 * scripted row count.
 *
 * The harness runs seven paths:
 *
 *     0  the state is zero: no call runs, and every cell keeps its value
 *     1  the state is 1 and the mode is 4, and the memory stays below 3000000:
 *        one memory call clears the state, and no list loads
 *     2  the mode is 0, the first memory is below 3000000, and the second is
 *        below 400000: two memory calls clear the state, and no list loads
 *     3  the memory is above 3000000, above 400000, and above 1900000, with the
 *        start index 5: SPFXBASE returns 12, SPFXFULL returns 3, the full flag
 *        is 1, the count is 15, and the index is 20
 *     4  the memory is below 3000000, then above 400000, then below 1900000,
 *        with the start index 2: only SPFXBASE loads, the full flag is 0, the
 *        count is 7, and the index is 9
 *     5  the memory is 3000000, 400000, and 1900000, with the mode 4: the two
 *        boundaries pass, the mode check is skipped, and only SPFXBASE loads
 *     6  the first memory is 3000000 and the second is 399999: two memory calls
 *        clear the state, and no list loads
 *     D  the exit marker
 *
 * The harness uses only BSS state, because the minimal image does not load the
 * initialized data page.  The expected text is therefore built from character
 * constants, and no string literal is compared.
 */
#pragma off (check_stack)

void sound_effect_list_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_00018d84;
int DAT_00019064;
int DAT_00019068;
int DAT_0001906c;
int DAT_0001d260;
int DAT_0001d2c8;
void *DAT_000062ac;
char DAT_0015224[0x24 * 0x40];

static char text[8];

static unsigned int memory_value[8];
static int memory_calls;

static char list_type[2][0x20];
static char list_name[2][0x20];
static char list_list[2][0x20];
static void *list_table[2];
static int list_stride[2];
static int list_count[2];
static int list_result[2];
static int list_calls;

static int match3(char *text, char a, char b, char c)
{
    return text[0] == a && text[1] == b && text[2] == c && text[3] == 0;
}

static int match8(char *text, char a, char b, char c, char d,
                  char e, char f, char g, char h)
{
    return text[0] == a && text[1] == b && text[2] == c && text[3] == d
        && text[4] == e && text[5] == f && text[6] == g && text[7] == h
        && text[8] == 0;
}

static int match_gen(char *text)
{
    return match3(text, 'G', 'E', 'N');
}

static int match_base(char *text)
{
    return match8(text, 'S', 'P', 'F', 'X', 'B', 'A', 'S', 'E');
}

static int match_full(char *text)
{
    return match8(text, 'S', 'P', 'F', 'X', 'F', 'U', 'L', 'L');
}

static int match_list(char *text)
{
    return match8(text, 'S', 'O', 'U', 'N', 'D', '_', 'F', 'X');
}

static void set_text(char *destination, char *source)
{
    while (*source != 0) {
        *destination = *source;
        destination = destination + 1;
        source = source + 1;
    }
    *destination = 0;
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

unsigned int memory_available(void *pool)
{
    unsigned int value;

    (void)pool;
    if (memory_calls < 8) {
        value = memory_value[memory_calls];
    } else {
        value = 0;
    }
    memory_calls = memory_calls + 1;
    return value;
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

static void reset(void)
{
    int index;

    DAT_0001d2c8 = 1;
    DAT_0001d260 = 0;
    DAT_00018d84 = 0;
    DAT_00019064 = 0x22222222;
    DAT_00019068 = 0x33333333;
    DAT_0001906c = 0x44444444;
    DAT_000062ac = (void *)0x11110000;
    for (index = 0; index < 0x24 * 0x40; ++index) {
        DAT_0015224[index] = 0;
    }
    memory_calls = 0;
    for (index = 0; index < 8; ++index) {
        memory_value[index] = 0;
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
}

static int untouched(void)
{
    return DAT_00019064 == 0x22222222
        && DAT_00019068 == 0x33333333
        && DAT_0001906c == 0x44444444;
}

static int disabled(void)
{
    return memory_calls == 0
        && list_calls == 0
        && DAT_0001d2c8 == 0
        && DAT_00018d84 == 7
        && untouched();
}

static int low_mode_four(void)
{
    return memory_calls == 1
        && list_calls == 0
        && DAT_0001d2c8 == 0
        && DAT_00018d84 == 0
        && untouched();
}

static int low_mode_other(void)
{
    return memory_calls == 2
        && list_calls == 0
        && DAT_0001d2c8 == 0
        && DAT_00018d84 == 0
        && untouched();
}

static int full_lists(void)
{
    return memory_calls == 3
        && list_calls == 2
        && DAT_0001d2c8 == 1
        && DAT_00019064 == 1
        && DAT_0001906c == 5
        && DAT_00019068 == 15
        && DAT_00018d84 == 20
        && match_gen(list_type[0])
        && match_base(list_name[0])
        && match_list(list_list[0])
        && list_table[0] == (void *)(DAT_0015224 + 5 * 0x24)
        && list_stride[0] == 0x24
        && list_count[0] == 0x24
        && match_gen(list_type[1])
        && match_full(list_name[1])
        && match_list(list_list[1])
        && list_table[1] == (void *)(DAT_0015224 + (5 + 12) * 0x24)
        && list_stride[1] == 0x24
        && list_count[1] == 0x24;
}

static int base_only(void)
{
    return memory_calls == 3
        && list_calls == 1
        && DAT_0001d2c8 == 1
        && DAT_00019064 == 0
        && DAT_0001906c == 2
        && DAT_00019068 == 7
        && DAT_00018d84 == 9
        && match_base(list_name[0])
        && list_table[0] == (void *)(DAT_0015224 + 2 * 0x24);
}

static int boundaries(void)
{
    return memory_calls == 3
        && list_calls == 1
        && DAT_0001d2c8 == 1
        && DAT_00019064 == 0
        && DAT_0001906c == 0
        && DAT_00019068 == 4
        && DAT_00018d84 == 4;
}

static int second_low(void)
{
    return memory_calls == 2
        && list_calls == 0
        && DAT_0001d2c8 == 0
        && DAT_00018d84 == 0
        && untouched();
}

int main(void)
{
    reset();
    DAT_0001d2c8 = 0;
    DAT_00018d84 = 7;
    sound_effect_list_init();
    put(disabled() ? '0' : 'x');

    reset();
    DAT_0001d260 = 4;
    memory_value[0] = 2000000;
    sound_effect_list_init();
    put(low_mode_four() ? '1' : 'x');

    reset();
    DAT_0001d260 = 0;
    memory_value[0] = 2000000;
    memory_value[1] = 100000;
    sound_effect_list_init();
    put(low_mode_other() ? '2' : 'x');

    reset();
    DAT_0001d260 = 0;
    DAT_00018d84 = 5;
    memory_value[0] = 4000000;
    memory_value[1] = 4000000;
    memory_value[2] = 2000000;
    list_result[0] = 12;
    list_result[1] = 3;
    sound_effect_list_init();
    put(full_lists() ? '3' : 'x');

    reset();
    DAT_0001d260 = 0;
    DAT_00018d84 = 2;
    memory_value[0] = 2000000;
    memory_value[1] = 400000;
    memory_value[2] = 1000000;
    list_result[0] = 7;
    sound_effect_list_init();
    put(base_only() ? '4' : 'x');

    reset();
    DAT_0001d260 = 4;
    memory_value[0] = 3000000;
    memory_value[1] = 400000;
    memory_value[2] = 1900000;
    list_result[0] = 4;
    sound_effect_list_init();
    put(boundaries() ? '5' : 'x');

    reset();
    DAT_0001d260 = 0;
    memory_value[0] = 3000000;
    memory_value[1] = 399999;
    sound_effect_list_init();
    put(second_low() ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
