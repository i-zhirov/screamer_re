/* The test harness for the rebuilt `menu_line_value_next` module.
 *
 * The harness holds the menu index, the selected-line table, the two line
 * tables, and the overlay word.  The `map_object_progress_sum` stub records its
 * call count and returns the configured float.
 *
 * The paths:
 *
 *     0  the sum 1.0 keeps the value index 2 and the overlay word 0
 *     1  the sum 0.0 raises the value index 2 to 3 and sets the overlay word
 *     2  the value index 5 at the count 5 wraps to 0
 *     3  the value index 4 at the count 5 rises to 5 and stays
 *     4  the second menu raises the value index of its line 2 from 0 to 1
 *     5  the sum -1.5 keeps the value index 2 and the overlay word 0
 *     6  the value index 0 at the count 0 wraps to 0
 *     7  the sum -0.0 counts as zero and raises the value index 1 to 2
 *     D  the exit marker
 *
 * The float values arrive from their bit patterns, because the minimal image
 * does not load the constant segment.
 */
#pragma off (check_stack)

#define LINE_COUNT 4

struct menu_line {
    int string_offset;          /* +0x00 */
    int item_id;                /* +0x04 */
    int value_index;            /* +0x08 */
    int value_count;            /* +0x0c */
};

void menu_line_value_next(void);
float map_object_progress_sum(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char *DAT_002de8[8];
int DAT_001fb08[8];
int DAT_0011fb60;
int DAT_0011fac0;

static char text[8];
static struct menu_line line_table_a[LINE_COUNT];
static struct menu_line line_table_b[LINE_COUNT];
static float progress_value;
static int progress_calls;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static float float_bits(unsigned int bits)
{
    union {
        float f;
        unsigned int u;
    } value;

    value.u = bits;
    return value.f;
}

float map_object_progress_sum(void)
{
    progress_calls = progress_calls + 1;
    return progress_value;
}

static void reset(void)
{
    struct menu_line *tables[2];
    int table;
    int index;

    tables[0] = line_table_a;
    tables[1] = line_table_b;
    for (table = 0; table < 2; table++) {
        for (index = 0; index < LINE_COUNT; index++) {
            tables[table][index].string_offset = index + 1;
            tables[table][index].item_id = index + 0x100;
            tables[table][index].value_index = 0;
            tables[table][index].value_count = 0;
        }
    }
    for (index = 0; index < 8; index++) {
        DAT_002de8[index] = 0;
        DAT_001fb08[index] = 0;
    }
    DAT_002de8[0] = (char *)line_table_a;
    DAT_002de8[1] = (char *)line_table_b;
    DAT_0011fb60 = 0;
    DAT_0011fac0 = 0;
    progress_value = 0.0f;
    progress_calls = 0;
}

static int others_clear(int menu, int row)
{
    struct menu_line *tables[2];
    int table;
    int index;

    tables[0] = line_table_a;
    tables[1] = line_table_b;
    for (table = 0; table < 2; table++) {
        for (index = 0; index < LINE_COUNT; index++) {
            if (table == menu && index == row) {
                continue;
            }
            if (tables[table][index].value_index != 0) {
                return 0;
            }
        }
    }
    return 1;
}

static int verify(int menu, int row, int before, int count,
                  unsigned int progress_bits, int expected_value,
                  int expected_flag)
{
    struct menu_line *line;

    reset();
    DAT_0011fb60 = menu;
    DAT_001fb08[menu] = row;
    if (menu == 0) {
        line = &line_table_a[row];
    } else {
        line = &line_table_b[row];
    }
    line->value_index = before;
    line->value_count = count;
    progress_value = float_bits(progress_bits);
    menu_line_value_next();
    return progress_calls == 1
        && line->value_index == expected_value
        && line->value_count == count
        && line->string_offset == row + 1
        && line->item_id == row + 0x100
        && others_clear(menu, row)
        && DAT_0011fac0 == expected_flag;
}

int main(void)
{
    put(verify(0, 1, 2, 5, 0x3f800000, 2, 0) ? '0' : 'x');
    put(verify(0, 1, 2, 5, 0x00000000, 3, 1) ? '1' : 'x');
    put(verify(0, 1, 5, 5, 0x00000000, 0, 1) ? '2' : 'x');
    put(verify(0, 1, 4, 5, 0x00000000, 5, 1) ? '3' : 'x');
    put(verify(1, 2, 0, 3, 0x00000000, 1, 1) ? '4' : 'x');
    put(verify(0, 1, 2, 5, 0xbfc00000, 2, 0) ? '5' : 'x');
    put(verify(0, 1, 0, 0, 0x00000000, 0, 1) ? '6' : 'x');
    put(verify(0, 0, 1, 9, 0x80000000, 2, 1) ? '7' : 'x');
    put('D');
    quit();
    return 0;
}
