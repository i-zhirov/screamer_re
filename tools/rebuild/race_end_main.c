/* The test harness for the rebuilt `race_end` module.
 *
 * The harness holds the input flag, the two copy sources, and the thirteen
 * stubs.  Each stub records its call order.  The argument stubs record their
 * values.  The harness runs the zero flag path and the non-zero flag path:
 *
 *     0  a zero flag runs all thirteen calls, sets the flag to 1, and copies
 *        the two final values; the arguments hold the data addresses
 *     1  a non-zero flag keeps its value, skips the gated call, and still
 *        copies the two final values
 *     D  the exit marker
 *
 * The order checks use explicit comparisons, because the initialized arrays of
 * the harness are not reliable under the link model of this project.
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_end(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

char DAT_000062b0[4];
char DAT_00006350[4];
char DAT_00006530[4];
int DAT_0018e3c;
unsigned int DAT_001d22c;
unsigned int DAT_001d274;
unsigned int DAT_001d230;

#define STEP(index, value) (order[index] == (value))

static char text[8];
static char order[16];
static int order_count;
static int arg_01;
static void *arg_03a;
static void *arg_03b;
static void *arg_06;
static void *arg_07;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void record(char step)
{
    if (order_count < 16) {
        order[order_count] = step;
    }
    order_count = order_count + 1;
}

void teardown_01(int value) { arg_01 = value; record(1); }
void teardown_02(void) { record(2); }
void teardown_03(void *a, void *b) { arg_03a = a; arg_03b = b; record(3); }
void teardown_04(void) { record(4); }
void teardown_05(void) { record(5); }
void teardown_06(void *a) { arg_06 = a; record(6); }
void teardown_07(void *a) { arg_07 = a; record(7); }
void teardown_08(void) { record(8); }
void teardown_09(void) { record(9); }
void teardown_10(void) { record(10); }
void teardown_11(void) { record(11); }
void teardown_12(void) { record(12); }
void teardown_13(void) { record(13); }

static void reset(void)
{
    int index;

    for (index = 0; index < 16; ++index) {
        order[index] = 0;
    }
    order_count = 0;
    arg_01 = -1;
    arg_03a = 0;
    arg_03b = 0;
    arg_06 = 0;
    arg_07 = 0;
    DAT_001d274 = 0;
    DAT_001d230 = 0;
}

int main(void)
{
    reset();
    DAT_0018e3c = 0;
    DAT_001d22c = 0x11223344;
    race_end();
    put(order_count == 13
        && STEP(0, 1) && STEP(1, 2) && STEP(2, 3) && STEP(3, 4)
        && STEP(4, 5) && STEP(5, 6) && STEP(6, 7) && STEP(7, 8)
        && STEP(8, 9) && STEP(9, 10) && STEP(10, 11) && STEP(11, 12)
        && STEP(12, 13)
        && arg_01 == 0
        && arg_03a == DAT_000062b0 && arg_03b == DAT_00006350
        && arg_06 == DAT_00006530 && arg_07 == DAT_00006530
        && DAT_0018e3c == 1
        && DAT_001d274 == 0x11223344 && DAT_001d230 == 1 ? '0' : 'x');

    reset();
    DAT_0018e3c = 5;
    DAT_001d22c = 0xaabbccdd;
    race_end();
    put(order_count == 12
        && STEP(0, 1) && STEP(1, 2) && STEP(2, 3) && STEP(3, 4)
        && STEP(4, 5) && STEP(5, 6) && STEP(6, 7) && STEP(7, 9)
        && STEP(8, 10) && STEP(9, 11) && STEP(10, 12) && STEP(11, 13)
        && arg_01 == 0
        && arg_03a == DAT_000062b0 && arg_03b == DAT_00006350
        && arg_06 == DAT_00006530 && arg_07 == DAT_00006530
        && DAT_0018e3c == 5
        && DAT_001d274 == 0xaabbccdd && DAT_001d230 == 5 ? '1' : 'x');

    put('D');
    quit();
    return 0;
}
