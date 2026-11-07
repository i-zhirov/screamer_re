/* The test harness for the rebuilt `championship_result_apply` module.
 *
 * The harness holds the two input flag bytes, the result flag, the selected
 * record index, the previous record index, the car number, the race time, the
 * points table, the three standings arrays, and the 0x2c-byte championship
 * records.
 *
 * The harness runs six paths:
 *
 *     0  the normal path on group 1: the car, the time, and the points stores,
 *        the point sum with the four 9-point fills, and the record advance
 *     1  the input bit 0x10 sets the result 1, and the threshold passes
 *     2  the point sum is below the threshold 42, so the result is 2
 *     3  group 5 adds no 9-point fill, and the low sum is below the threshold
 *     4  the abort bit 0x20 leaves every global unchanged
 *     5  the point sum equals the threshold, so the result stays 0
 *     D  the exit marker
 */
#pragma off (check_stack)

void championship_result_apply(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

struct champ_record {
    int type;
    int value;
    int title_index;
    int group;
    int field_10;
    int field_14;
    char name[12];
    int field_24;
    int font;
};

unsigned char DAT_00118e1c;
unsigned char DAT_00118e1d;
int DAT_00120804;
int DAT_001203dc;
int DAT_001203e0;
int DAT_0011d270;
int DAT_0011d268;
int DAT_001021f0[32];
int DAT_001203e8[6];
int DAT_00120400[6];
int DAT_00120418[6];
struct champ_record DAT_001022bc[12];

static char text[8];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void reset(void)
{
    int index;

    DAT_00118e1c = 0;
    DAT_00118e1d = 0;
    DAT_00120804 = 0x7f;
    DAT_001203dc = 0;
    DAT_001203e0 = 0x7f;
    DAT_0011d270 = 0;
    DAT_0011d268 = 0;
    for (index = 0; index < 32; ++index) {
        DAT_001021f0[index] = 0;
    }
    for (index = 0; index < 6; ++index) {
        DAT_001203e8[index] = 0;
        DAT_00120400[index] = -1;
        DAT_00120418[index] = 0;
    }
    for (index = 0; index < 12; ++index) {
        DAT_001022bc[index].type = 0;
        DAT_001022bc[index].value = 0;
        DAT_001022bc[index].title_index = 0;
        DAT_001022bc[index].group = 0;
        DAT_001022bc[index].field_10 = 0;
        DAT_001022bc[index].field_14 = 0;
        DAT_001022bc[index].name[0] = 0;
        DAT_001022bc[index].field_24 = 0;
        DAT_001022bc[index].font = 0;
    }
}

int main(void)
{
    reset();
    DAT_0011d270 = 2;
    DAT_0011d268 = 0x41480000;
    DAT_001021f0[2] = 7;
    DAT_001203dc = 4;
    DAT_001022bc[4].field_24 = 1;
    DAT_001022bc[5].value = 0;
    championship_result_apply();
    put(DAT_00120804 == 0 && DAT_001203dc == 5 && DAT_001203e0 == 5
        && DAT_00120400[1] == 2 && DAT_001203e8[1] == 0x41480000
        && DAT_00120418[1] == 7 && DAT_00120418[0] == 0 ? '0' : 'x');

    reset();
    DAT_00118e1d = 0x10;
    DAT_0011d270 = 0;
    DAT_0011d268 = 0x41f00000;
    DAT_001021f0[0] = 24;
    DAT_001022bc[0].field_24 = 0;
    DAT_001022bc[1].value = 0;
    championship_result_apply();
    put(DAT_00120804 == 1 && DAT_001203dc == 1 && DAT_001203e0 == 1
        && DAT_00120400[0] == 0 && DAT_001203e8[0] == 0x41f00000
        && DAT_00120418[0] == 24 ? '1' : 'x');

    reset();
    DAT_0011d270 = 1;
    DAT_0011d268 = 0x41200000;
    DAT_001021f0[1] = 9;
    DAT_001203dc = 2;
    DAT_001022bc[2].field_24 = 2;
    DAT_001022bc[3].value = 42;
    championship_result_apply();
    put(DAT_00120804 == 2 && DAT_001203dc == 3 && DAT_001203e0 == 3
        && DAT_00120400[2] == 1 && DAT_00120418[2] == 9 ? '2' : 'x');

    reset();
    DAT_0011d270 = 3;
    DAT_0011d268 = 0x40a00000;
    DAT_001021f0[3] = 5;
    DAT_001203dc = 5;
    DAT_001022bc[5].field_24 = 5;
    DAT_001022bc[6].value = 6;
    championship_result_apply();
    put(DAT_00120804 == 2 && DAT_001203dc == 6 && DAT_001203e0 == 6
        && DAT_00120400[5] == 3 && DAT_00120418[5] == 5 ? '3' : 'x');

    reset();
    DAT_00118e1c = 0x20;
    DAT_00120804 = 0x55;
    DAT_001203dc = 3;
    DAT_001203e0 = 0x66;
    DAT_00120400[0] = 0x1234;
    DAT_0011d270 = 1;
    championship_result_apply();
    put(DAT_00120804 == 0x55 && DAT_001203dc == 3 && DAT_001203e0 == 0x66
        && DAT_00120400[0] == 0x1234 && DAT_00120418[0] == 0 ? '4' : 'x');

    reset();
    DAT_0011d270 = 0;
    DAT_0011d268 = 0x42480000;
    DAT_001021f0[0] = 24;
    DAT_001022bc[0].field_24 = 0;
    DAT_001022bc[1].value = 69;
    championship_result_apply();
    put(DAT_00120804 == 0 && DAT_00120418[0] == 24
        && DAT_001203dc == 1 && DAT_001203e0 == 1 ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
