/* The test harness for the rebuilt `race_timeout` module.
 *
 * The harness holds the three state words and the frame delta stub.  The stub
 * records the two arguments and returns the configured delta.  The harness
 * runs the module with a delta above 1000 and then with a delta below:
 *
 *     0  with the delta 1500: the counter resets and the timeout flag sets
 *     1  with the delta 500: the counter and the flag stay
 *     D  the exit marker
 *
 * The markers go through the data buffer `text`, because the `print` helper of
 * this harness writes garbage for a string literal (the same fault shows in the
 * output of the driver at work/rebuild/out.txt).
 */
#pragma off (check_stack)

void race_timeout(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

unsigned int DAT_000107fd;
unsigned int DAT_0011d228;
unsigned int DAT_00101e04;

static char text[8];
static unsigned int delta_result;
static unsigned int got_a;
static unsigned int got_b;

static void put(char a)
{
    text[0] = a;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

unsigned int frame_delta(unsigned int a, unsigned int b)
{
    got_a = a;
    got_b = b;
    return delta_result;
}

int main(void)
{
    DAT_000107fd = 1000;
    DAT_0011d228 = 500;
    DAT_00101e04 = 0;
    delta_result = 1500;
    race_timeout();
    put(got_a == 500 && got_b == 1000
        && DAT_0011d228 == 1000 && DAT_00101e04 == 1 ? '0' : 'x');
    DAT_0011d228 = 500;
    DAT_00101e04 = 0;
    delta_result = 500;
    race_timeout();
    put(DAT_0011d228 == 500 && DAT_00101e04 == 0 ? '1' : 'x');
    put('D');
    quit();
    return 0;
}
