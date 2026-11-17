/* The test harness for the rebuilt `name_entry_update` module.
 *
 * The harness holds the letter index, the 0x50 map object records at
 * `DAT_001db78`, the three marker cells, and the three float constants.  The
 * two callees are stubs that record the call order and the call arguments.
 *
 * The harness runs these paths:
 *
 *     0  the index 1: the select receives the key 0x42, the letter update runs
 *        on all 0x50 records with the stride 0x64, and the marker reaches
 *        -1076 and 4369 with the heading 0.24166098
 *     1  the index 0: the select receives the key 0x41, the two integer cells
 *        clear, and the default 0x1194 reaches the second cell
 *     2  the index 13: the sine term reaches zero and the cosine term reaches
 *        -4500, so the truncate keeps the exact zero
 *     3  the index 25: the upper bound places the marker at 1076 and 4369
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because an FPU memory
 * operand of a literal reads the constant pool at a low address in this
 * toolchain.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50

void name_entry_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_0011d23c;
char DAT_001db78[RECORD_COUNT * RECORD_SIZE];
int DAT_001203a8;
int DAT_001203b0;
float DAT_00120394;

float name_entry_cos_sine_scale;
float name_entry_marker_scale;
float name_entry_heading_scale;

static char text[8];
static int sequence;
static int select_calls;
static int select_key;
static int select_order;
static int letter_calls;
static int first_letter_order;
static char *letter_record[RECORD_COUNT];

void map_object_line_select(int key)
{
    select_calls = select_calls + 1;
    select_key = key;
    select_order = sequence;
    sequence = sequence + 1;
}

void name_entry_letter_update(char *record)
{
    if (letter_calls < RECORD_COUNT) {
        letter_record[letter_calls] = record;
    }
    if (first_letter_order < 0) {
        first_letter_order = sequence;
    }
    sequence = sequence + 1;
    letter_calls = letter_calls + 1;
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

static unsigned int bits(float value)
{
    return *(unsigned int *)&value;
}

static void set_bits_float(float *value, unsigned int pattern)
{
    *(unsigned int *)value = pattern;
}

static void clear_records(void)
{
    int index;

    for (index = 0; index < RECORD_COUNT * RECORD_SIZE; index++) {
        DAT_001db78[index] = 0;
    }
}

static void reset(void)
{
    int index;

    clear_records();
    DAT_0011d23c = 0;
    DAT_001203a8 = 0x1111;
    DAT_001203b0 = 0x2222;
    DAT_00120394 = 123.0f;
    set_bits_float(&name_entry_cos_sine_scale, 0xbe7775fa);
    set_bits_float(&name_entry_marker_scale, 0x458ca000);
    set_bits_float(&name_entry_heading_scale, 0x3e7775fa);
    sequence = 0;
    select_calls = 0;
    select_key = 0;
    select_order = -1;
    letter_calls = 0;
    first_letter_order = -1;
    for (index = 0; index < RECORD_COUNT; index++) {
        letter_record[index] = 0;
    }
}

static int records_ok(void)
{
    return letter_calls == RECORD_COUNT
        && letter_record[0] == DAT_001db78
        && letter_record[1] == DAT_001db78 + RECORD_SIZE
        && letter_record[RECORD_COUNT - 1]
            == DAT_001db78 + (RECORD_COUNT - 1) * RECORD_SIZE;
}

static int order_ok(void)
{
    return select_calls == 1 && select_order >= 0
        && first_letter_order > select_order;
}

int main(void)
{
    int ok;

    reset();
    DAT_0011d23c = 1;
    name_entry_update();
    ok = order_ok() && records_ok() && select_key == 0x42
        && DAT_001203a8 == -1076 && DAT_001203b0 == 4369
        && bits(DAT_00120394) == 0x3e7775fa;
    put(ok ? '0' : 'x');

    reset();
    DAT_0011d23c = 0;
    name_entry_update();
    ok = order_ok() && records_ok() && select_key == 0x41
        && DAT_001203a8 == 0 && DAT_001203b0 == 0x1194
        && bits(DAT_00120394) == 0;
    put(ok ? '1' : 'x');

    reset();
    DAT_0011d23c = 13;
    name_entry_update();
    ok = order_ok() && records_ok() && select_key == 0x4e
        && DAT_001203a8 == 0 && DAT_001203b0 == -4500
        && bits(DAT_00120394) == 0x40490fdb;
    put(ok ? '2' : 'x');

    reset();
    DAT_0011d23c = 25;
    name_entry_update();
    ok = order_ok() && records_ok() && select_key == 0x5a
        && DAT_001203a8 == 1076 && DAT_001203b0 == 4369
        && bits(DAT_00120394) == 0x40c1542b;
    put(ok ? '3' : 'x');

    put('D');
    quit();
    return 0;
}
