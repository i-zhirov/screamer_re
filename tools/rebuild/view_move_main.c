/* The test harness for the rebuilt `view_move` module.
 *
 * The harness holds the three view stubs.  Each stub records its arguments.
 * The harness calls the module and checks the arguments:
 *
 *     0  the first stub ran with `b`
 *     1  the second stub ran with `0x62b0` and `c >> 8`
 *     2  the third stub ran twice with `(a + a) >> 8`
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void view_move(int a, int b, int c);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
static int got_b;
static int got_1a;
static int got_1b;
static int got_2a;
static int got_2b;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

void view_move_0(int a)
{
    got_b = a;
}

void view_move_1(int a, int b)
{
    got_1a = a;
    got_1b = b;
}

void view_move_2(int a, int b)
{
    got_2a = a;
    got_2b = b;
}

int main(void)
{
    int v;

    v = (0x30 + 0x30) >> 8;
    view_move(0x30, 0x51, 0x100);
    put(got_b == 0x51 ? '0' : 'x');
    put(got_1a == 0x62b0 && got_1b == 0x100 >> 8 ? '1' : 'x');
    put(got_2a == v && got_2b == v ? '2' : 'x');
    put('D');
    quit();
    return 0;
}
