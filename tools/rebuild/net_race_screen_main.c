/* The test harness for the rebuilt `net_race_screen` dispatcher.
 *
 * The harness holds the two menu tables, the menu index, the state word, and
 * the eighteen step stubs.  The stubs count their calls.  The harness runs the
 * overlay entry and the menu ids 0, 0x1c, 0x1f, 0x4d2, and 0x30:
 *
 *     0  the state 2 runs the overlay and no case
 *     1  the id 0 runs case 0
 *     2  the id 0x1c with the low word runs the low case
 *     3  the id 0x1f runs case 0x1f
 *     4  the id 0x4d2 runs case 0x4d2
 *     5  the id 0x30 runs no case
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void net_race_screen(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_0011fac4;
unsigned int DAT_0011fb60;
unsigned int DAT_0011fb08[4];
unsigned int DAT_00102de8[4];
unsigned int DAT_00120880;

static char text[8];
static unsigned char record_table[0x20];
static int calls[18];

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void net_race_overlay(void) { calls[0] = calls[0] + 1; }
void case_00(void) { calls[1] = calls[1] + 1; }
void case_01(void) { calls[2] = calls[2] + 1; }
void case_02(void) { calls[3] = calls[3] + 1; }
void case_03(void) { calls[4] = calls[4] + 1; }
void case_04(void) { calls[5] = calls[5] + 1; }
void case_15(void) { calls[6] = calls[6] + 1; }
void case_18(void) { calls[7] = calls[7] + 1; }
void case_19(void) { calls[8] = calls[8] + 1; }
void case_1a(void) { calls[9] = calls[9] + 1; }
void case_1b(void) { calls[10] = calls[10] + 1; }
void case_1c_low(void) { calls[11] = calls[11] + 1; }
void case_1c_high(void) { calls[12] = calls[12] + 1; }
void case_1d(void) { calls[13] = calls[13] + 1; }
void case_1e(void) { calls[14] = calls[14] + 1; }
void case_1f(void) { calls[15] = calls[15] + 1; }
void case_4d2(void) { calls[16] = calls[16] + 1; }

static void reset(void)
{
    int index;

    for (index = 0; index < 18; ++index) {
        calls[index] = 0;
    }
    for (index = 0; index < 0x20; ++index) {
        record_table[index] = 0;
    }
    DAT_0011fac4 = 0;
    DAT_0011fb60 = 0;
    DAT_0011fb08[0] = 0;
    DAT_00102de8[0] = (unsigned int)record_table;
    DAT_00120880 = 0;
}

static int only_call(int index)
{
    int i;
    int ok;

    ok = calls[index] == 1;
    for (i = 0; i < 17; ++i) {
        if (i != index && calls[i] != 0) {
            ok = 0;
        }
    }
    return ok;
}

static int no_calls(void)
{
    int i;

    for (i = 0; i < 17; ++i) {
        if (calls[i] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    reset();
    DAT_0011fac4 = 2;
    net_race_screen();
    put(only_call(0) ? '0' : 'x');

    reset();
    *(unsigned int *)(record_table + 4) = 0x00;
    net_race_screen();
    put(only_call(1) ? '1' : 'x');

    reset();
    *(unsigned int *)(record_table + 4) = 0x1c;
    DAT_00120880 = 0x100;
    net_race_screen();
    put(only_call(11) ? '2' : 'x');

    reset();
    *(unsigned int *)(record_table + 4) = 0x1f;
    net_race_screen();
    put(only_call(15) ? '3' : 'x');

    reset();
    *(unsigned int *)(record_table + 4) = 0x4d2;
    net_race_screen();
    put(only_call(16) ? '4' : 'x');

    reset();
    *(unsigned int *)(record_table + 4) = 0x30;
    net_race_screen();
    put(no_calls() ? '5' : 'x');

    put('D');
    quit();
    return 0;
}
