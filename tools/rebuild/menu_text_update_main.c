/* The test harness for the rebuilt `menu_text_update` module.
 *
 * The harness holds the menu index, the menu key, the line table, the string
 * table, the line descriptor, the 0x50 map object records, and the frame
 * phase.  The five callees are stubs that record their arguments.
 *
 * The harness runs seven paths:
 *
 *     0  the key differs: the reset runs on 0x50 records, three active lines
 *        are placed with the rows 5, 7, and 9, the selection flag is 1, 0, 1,
 *        and the update runs on 0x50 records
 *     1  the key matches: the reset and the placement stay out, and only the
 *        selection, the phase, and the update run
 *     2  the key is -2: the placement runs and the finish path runs on 0x50
 *        records, the key returns to the menu index, and the overlay word clears
 *     3  the phase crosses 8 pi: the phase wraps to zero
 *     4  the counter is 7: the eighth table entry places the lines and the
 *        counter wraps to zero
 *     5  the menu holds no line: the count is zero, the row is 8, and no line
 *        is placed
 *     6  the update sets the key to -2: the function keeps the key
 *     D  the exit marker
 *
 * The float constants arrive from their bit patterns, because the minimal
 * image does not load the constant segment.  The phase checks compare the bit
 * patterns.
 */
#pragma off (check_stack)

#define RECORD_SIZE 0x64
#define RECORD_COUNT 0x50
#define LINE_SIZE 0x10

struct map_line {
    int line;
    int field4;
    int field8;
    int centered;
    int flag10;
    int start_x;
    int start_y;
    int start_z;
    int amplitude_x;
    int amplitude_y;
    float angle;
    float timer;
};

void menu_text_update(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_002de8[8];
int DAT_001fb08[8];
char *DAT_001fb80[16];
char DAT_001db78[RECORD_COUNT * RECORD_SIZE];
int DAT_0011fb60;
int DAT_0011fb64;
int DAT_0011fb6c;
int DAT_0011fabc;
int DAT_0011fac0;
int DAT_0000221c[40];
struct map_line DAT_0011fad4;
float DAT_0018e08;
float DAT_0011d248;
float menu_text_phase_rate;
float menu_text_phase_bound;

static char text[8];
static char line_records[4 * LINE_SIZE];
static char string_a[2] = "A";
static char string_b[2] = "B";
static char string_c[2] = "C";

static int reset_calls;
static int reset_last_line;
static int place_calls;
static int select_calls;
static int select_last;
static int update_calls;
static int finish_calls;
static int update_sets_mode;

struct place_call {
    int line;
    int field4;
    int flag10;
    char *text;
};

static struct place_call places[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void set_bits_float(float *p, unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    *p = value.f;
}

static unsigned int float_bits(float value)
{
    union {
        float f;
        unsigned int u;
    } field;

    field.f = value;
    return field.u;
}

static void clear_bytes(char *p, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        p[index] = 0;
    }
}

static void set_line(int index, int offset0, int offset8)
{
    *(int *)(line_records + index * LINE_SIZE) = offset0;
    *(int *)(line_records + index * LINE_SIZE + 8) = offset8;
}

void map_object_record_init(char *record, int line)
{
    reset_calls = reset_calls + 1;
    reset_last_line = line;
    *(int *)record = line;
    *(int *)(record + 8) = -1;
}

void map_object_place(struct map_line *line, char *text)
{
    int index;

    index = place_calls;
    if (index < 8) {
        places[index].line = line->line;
        places[index].field4 = line->field4;
        places[index].flag10 = line->flag10;
        places[index].text = text;
    }
    place_calls = place_calls + 1;
}

void map_object_line_select(int entry)
{
    select_calls = select_calls + 1;
    select_last = entry;
}

void map_object_update(char *record)
{
    (void)record;
    update_calls = update_calls + 1;
    if (update_sets_mode != 0) {
        DAT_0011fb64 = -2;
    }
}

void map_object_finish(char *record)
{
    (void)record;
    finish_calls = finish_calls + 1;
}

static void reset_case(void)
{
    int index;

    clear_bytes(DAT_001db78, RECORD_COUNT * RECORD_SIZE);
    clear_bytes(line_records, sizeof(line_records));
    clear_bytes((char *)&DAT_0011fad4, sizeof(DAT_0011fad4));

    set_line(0, 0, 0);
    set_line(1, 1, 0);
    set_line(2, 2, 0);
    set_line(3, -1, 0);
    DAT_002de8[1] = line_records;
    DAT_001fb80[0] = string_a;
    DAT_001fb80[1] = string_b;
    DAT_001fb80[2] = string_c;

    DAT_0011fb60 = 1;
    DAT_0011fb64 = 0;
    DAT_0011fb6c = 0;
    DAT_0011fabc = 0;
    DAT_0011fac0 = 1;
    DAT_001fb08[1] = 1;

    for (index = 0; index < 40; index++) {
        DAT_0000221c[index] = 100 + index * 10;
    }
    DAT_0000221c[35] = 700;
    DAT_0000221c[36] = 800;
    DAT_0000221c[37] = 900;
    DAT_0000221c[38] = 70;
    DAT_0000221c[39] = 80;

    set_bits_float(&DAT_0018e08, 0x3f000000);
    set_bits_float(&DAT_0011d248, 0);
    set_bits_float(&menu_text_phase_rate, 0x4116cbe4);
    set_bits_float(&menu_text_phase_bound, 0x41c90fdb);

    reset_calls = 0;
    reset_last_line = 0;
    place_calls = 0;
    select_calls = 0;
    select_last = 0;
    update_calls = 0;
    finish_calls = 0;
    update_sets_mode = 0;
}

int main(void)
{
    reset_case();
    menu_text_update();
    put(reset_calls == 0x50
        && reset_last_line == 0x4f
        && DAT_0011fb6c == 3
        && place_calls == 3
        && places[0].line == 0
        && places[0].field4 == 5
        && places[0].flag10 == 1
        && places[0].text == string_a
        && places[1].line == 1
        && places[1].field4 == 7
        && places[1].flag10 == 0
        && places[1].text == string_b
        && places[2].line == 2
        && places[2].field4 == 9
        && places[2].flag10 == 1
        && places[2].text == string_c
        && DAT_0011fad4.line == 3
        && DAT_0011fad4.field4 == 11
        && DAT_0011fad4.centered == 1
        && float_bits(DAT_0011fad4.angle) == 0x3db851ec
        && DAT_0011fad4.timer == 0
        && DAT_0011fad4.start_x == 100
        && DAT_0011fad4.start_y == 110
        && DAT_0011fad4.start_z == 120
        && DAT_0011fad4.amplitude_x == 130
        && DAT_0011fad4.amplitude_y == 140
        && DAT_0011fabc == 1
        && select_calls == 1
        && select_last == 1
        && update_calls == 0x50
        && finish_calls == 0
        && DAT_0011fb64 == 1
        && float_bits(DAT_0011d248) == 0x4096cbe4 ? '0' : 'x');

    menu_text_update();
    put(reset_calls == 0x50
        && place_calls == 3
        && DAT_0011fabc == 1
        && select_calls == 2
        && update_calls == 0xa0
        && DAT_0011fb64 == 1
        && float_bits(DAT_0011d248) == 0x4116cbe4 ? '1' : 'x');

    reset_case();
    DAT_0011fb64 = -2;
    menu_text_update();
    put(reset_calls == 0x50
        && place_calls == 3
        && select_calls == 1
        && update_calls == 0
        && finish_calls == 0x50
        && DAT_0011fb64 == 1
        && DAT_0011fac0 == 0 ? '2' : 'x');

    reset_case();
    DAT_0011fb64 = 1;
    set_bits_float(&DAT_0011d248, 0x41c80000);
    set_bits_float(&DAT_0018e08, 0x3f800000);
    menu_text_update();
    put(reset_calls == 0
        && place_calls == 0
        && update_calls == 0x50
        && float_bits(DAT_0011d248) == 0 ? '3' : 'x');

    reset_case();
    DAT_0011fabc = 7;
    menu_text_update();
    put(DAT_0011fabc == 0
        && DAT_0011fad4.start_x == 700
        && DAT_0011fad4.start_y == 800
        && DAT_0011fad4.start_z == 900
        && DAT_0011fad4.amplitude_x == 70
        && DAT_0011fad4.amplitude_y == 80
        && place_calls == 3
        && DAT_0011fb6c == 3 ? '4' : 'x');

    reset_case();
    set_line(0, -1, 0);
    menu_text_update();
    put(DAT_0011fb6c == 0
        && DAT_0011fad4.field4 == 8
        && DAT_0011fad4.line == 0
        && place_calls == 0
        && reset_calls == 0x50
        && update_calls == 0x50 ? '5' : 'x');

    reset_case();
    update_sets_mode = 1;
    menu_text_update();
    put(DAT_0011fb64 == -2 ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
