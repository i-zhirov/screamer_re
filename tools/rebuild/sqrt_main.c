/* The test harness for the rebuilt `sqrt` module.
 *
 * The harness builds every double from its bit pattern, because the minimal
 * image does not load the constant segment.  The paths:
 *
 *     0  the argument 9.0 returns 3.0
 *     1  the argument 0.0 returns 0.0
 *     2  the argument -1.0 returns 0.0
 *     3  the argument 2.0 returns the rounded square root
 *     4  the argument 0.25 returns 0.5
 *     5  the argument 1000000.0 returns 1000.0
 *     6  the argument 100.0 returns 10.0
 *     7  the argument -0.0 returns -0.0 through the core
 *     D  the exit marker
 *
 * Every path compares the two 32-bit words of the result, because the square
 * root from the FPU is exactly rounded.
 */
#pragma off (check_stack)

double sqrt(double value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[4];

static double make_double(unsigned int high, unsigned int low)
{
    union {
        double value;
        unsigned int word[2];
    } bits;

    bits.word[0] = low;
    bits.word[1] = high;
    return bits.value;
}

static int same_double(double actual, double expected)
{
    union {
        double value;
        unsigned int word[2];
    } left;
    union {
        double value;
        unsigned int word[2];
    } right;

    left.value = actual;
    right.value = expected;
    return left.word[0] == right.word[0] && left.word[1] == right.word[1];
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    print(text);
}

static void check(char marker, unsigned int high, unsigned int low,
    unsigned int want_high, unsigned int want_low)
{
    double result;

    result = sqrt(make_double(high, low));
    put(same_double(result, make_double(want_high, want_low)) ? marker : 'x');
}

int main(void)
{
    check('0', 0x40220000, 0x00000000, 0x40080000, 0x00000000);
    check('1', 0x00000000, 0x00000000, 0x00000000, 0x00000000);
    check('2', 0xbff00000, 0x00000000, 0x00000000, 0x00000000);
    check('3', 0x40000000, 0x00000000, 0x3ff6a09e, 0x667f3bcd);
    check('4', 0x3fd00000, 0x00000000, 0x3fe00000, 0x00000000);
    check('5', 0x412e8480, 0x00000000, 0x408f4000, 0x00000000);
    check('6', 0x40590000, 0x00000000, 0x40240000, 0x00000000);
    check('7', 0x80000000, 0x00000000, 0x80000000, 0x00000000);

    put('D');
    quit();
    return 0;
}
