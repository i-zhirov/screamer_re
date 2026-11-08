/* The test harness for the rebuilt `race_cleanup` module.
 *
 * The harness holds the imported data words and the twelve stubs.  Each stub
 * records its call order.  The first two argument stubs record both arguments,
 * and the last two record the one argument.  The harness checks the call
 * sequence and the arguments:
 *
 *     0  all twelve stubs ran in the original order
 *     1  the two-argument and one-argument stubs received the data addresses
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_cleanup(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_000062ac;
char DAT_000062b0[4];
char DAT_00006530[4];
char DAT_00006350[4];
char DAT_00000fed[4];

static char text[8];
static char order[12];
static int order_count;
static void *arg_05a;
static void *arg_05b;
static void *arg_06a;
static void *arg_06b;
static void *arg_10;
static void *arg_12;
static char block_10;

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
    if (order_count < 12) {
        order[order_count] = step;
    }
    order_count = order_count + 1;
}

void cleanup_01(void) { record(1); }
void cleanup_02(void) { record(2); }
void cleanup_03(void) { record(3); }
void cleanup_04(void) { record(4); }
void cleanup_05(void *a, void *b) { arg_05a = a; arg_05b = b; record(5); }
void cleanup_06(void *a, void *b) { arg_06a = a; arg_06b = b; record(6); }
void cleanup_07(void) { record(7); }
void cleanup_08(void) { record(8); }
void cleanup_09(void) { record(9); }
void cleanup_10(void *a) { arg_10 = a; record(10); }
void cleanup_11(void) { record(11); }
void cleanup_12(void *a) { arg_12 = a; record(12); }

static int in_order(void)
{
    int index;

    if (order_count != 12) {
        return 0;
    }
    for (index = 0; index < 12; ++index) {
        if (order[index] != index + 1) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    DAT_000062ac = &block_10;
    order_count = 0;
    race_cleanup();
    put(in_order() ? '0' : 'x');
    put(arg_05a == DAT_00006530 && arg_05b == DAT_00006350
        && arg_06a == DAT_00006530 && arg_06b == DAT_00000fed
        && arg_10 == DAT_000062ac && arg_12 == DAT_000062b0 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
