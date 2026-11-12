/* The test harness for the rebuilt `trig_range_reduce` module.
 *
 * The harness holds the 2 pi constant `trig_two_pi`.  Every double comes from
 * its bit pattern, because the minimal image does not load the constant
 * segment.  Each path compares the result with a tolerance of 0.0001, because
 * the subtraction loop need not land on the same bit pattern as the stated
 * decimal value.
 *
 * The paths:
 *
 *     0  the value 0.0 keeps 0.0
 *     1  the value 1.0 stays below one turn: it keeps 1.0
 *     2  the value -1.0 stays above minus one turn: it keeps -1.0
 *     3  the value 2 pi reduces to 0.0
 *     4  the value 7.0 reduces to 0.71681469
 *     5  the value -7.0 reduces to -0.71681469
 *     6  the value 20.0 reduces to 1.15044408
 *     7  the value 2 pi plus 0.5 reduces to 0.5
 *     D  the exit marker
 */
#pragma off (check_stack)

double trig_range_reduce(double value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

double trig_two_pi;

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

static double absolute(double value)
{
    return value < 0.0 ? -value : value;
}

static int close(double value, double expected)
{
    double tolerance;

    tolerance = make_double(0x3f1a36e2, 0xeb1c432d);
    return absolute(value - expected) < tolerance;
}

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    print(text);
}

static void check(char marker, unsigned int value_high, unsigned int value_low,
    unsigned int want_high, unsigned int want_low)
{
    double result;

    result = trig_range_reduce(make_double(value_high, value_low));
    put(close(result, make_double(want_high, want_low)) ? marker : 'x');
}

int main(void)
{
    trig_two_pi = make_double(0x401921fb, 0x54442d18);

    check('0', 0x00000000, 0x00000000, 0x00000000, 0x00000000);
    check('1', 0x3ff00000, 0x00000000, 0x3ff00000, 0x00000000);
    check('2', 0xbff00000, 0x00000000, 0xbff00000, 0x00000000);
    check('3', 0x401921fb, 0x54442d18, 0x00000000, 0x00000000);
    check('4', 0x401c0000, 0x00000000, 0x3fe6f025, 0x5dde9740);
    check('5', 0xc01c0000, 0x00000000, 0xbfe6f025, 0x5dde9740);
    check('6', 0x40340000, 0x00000000, 0x3ff26838, 0x0ccde2df);
    check('7', 0x401b21fb, 0x54442d18, 0x3fe00000, 0x00000000);

    put('D');
    quit();
    return 0;
}
