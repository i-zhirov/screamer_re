/* The test harness for the rebuilt `championship_record_select` module.
 *
 * The harness holds the selected and previous record indices, the point sum,
 * the six standings car slots, the result flag, and the 0x2c-byte championship
 * records.
 *
 * The harness runs seven paths:
 *
 *     0  a normal row with a low car value selects the win record and keeps
 *        the current record in the previous slot
 *     1  a car value of 4 selects the lose record and the record before the
 *        current one
 *     2  a set result flag selects the lose record
 *     3  a title index of 5 with a car value other than 1 selects the lose
 *        record
 *     4  a title index of 5 with the car value 1 selects the win record
 *     5  the last row with the point sum at the value selects the win record
 *        in both slots and clears the six standings slots
 *     6  the last row with the point sum below the value selects the lose
 *        record
 *     D  the exit marker
 */
#pragma off (check_stack)

void championship_record_select(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct champ_record {
    int type;
    int value;
    int title_index;
    int group;
    int win_next;
    int lose_next;
    char name[12];
    int row;
    int font;
};

int DAT_001203dc;
int DAT_001203e0;
int DAT_001203e4;
int DAT_00120400[6];
int DAT_00120804;
struct champ_record DAT_001022bc[8];

static char text[8];

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void clear_bytes(char *base, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        base[index] = 0;
    }
}

static void reset(void)
{
    int index;

    clear_bytes((char *)DAT_001022bc, sizeof(DAT_001022bc));
    for (index = 0; index < 6; index = index + 1) {
        DAT_00120400[index] = 9;
    }
    DAT_001203dc = 0;
    DAT_001203e0 = 0x55;
    DAT_001203e4 = 0;
    DAT_00120804 = 0;
    DAT_001022bc[0].type = 3;
    DAT_001022bc[0].value = 50;
    DAT_001022bc[0].title_index = 1;
    DAT_001022bc[0].group = 2;
    DAT_001022bc[0].win_next = 3;
    DAT_001022bc[0].lose_next = 4;
    DAT_001022bc[0].row = 0;
    DAT_001022bc[0].font = 0;
}

int main(void)
{
    int ok;

    reset();
    DAT_00120400[0] = 2;
    championship_record_select();
    ok = DAT_001203dc == 3 && DAT_001203e0 == 0;
    put(ok ? '0' : 'x');

    reset();
    DAT_00120400[0] = 4;
    championship_record_select();
    ok = DAT_001203dc == 4 && DAT_001203e0 == -1;
    put(ok ? '1' : 'x');

    reset();
    DAT_00120400[0] = 2;
    DAT_00120804 = 1;
    championship_record_select();
    ok = DAT_001203dc == 4 && DAT_001203e0 == -1;
    put(ok ? '2' : 'x');

    reset();
    DAT_00120400[0] = 2;
    DAT_001022bc[0].title_index = 5;
    championship_record_select();
    ok = DAT_001203dc == 4 && DAT_001203e0 == -1;
    put(ok ? '3' : 'x');

    reset();
    DAT_00120400[0] = 1;
    DAT_001022bc[0].title_index = 5;
    championship_record_select();
    ok = DAT_001203dc == 3 && DAT_001203e0 == 0;
    put(ok ? '4' : 'x');

    reset();
    DAT_00120400[5] = 2;
    DAT_001022bc[0].row = 5;
    DAT_001203e4 = 50;
    championship_record_select();
    ok = DAT_001203dc == 3
        && DAT_001203e0 == 3
        && DAT_00120400[0] == -1
        && DAT_00120400[1] == -1
        && DAT_00120400[2] == -1
        && DAT_00120400[3] == -1
        && DAT_00120400[4] == -1
        && DAT_00120400[5] == -1;
    put(ok ? '5' : 'x');

    reset();
    DAT_00120400[5] = 2;
    DAT_001022bc[0].row = 5;
    DAT_001203e4 = 49;
    championship_record_select();
    ok = DAT_001203dc == 4 && DAT_001203e0 == -1;
    put(ok ? '6' : 'x');

    put('D');
    quit();
    return 0;
}
