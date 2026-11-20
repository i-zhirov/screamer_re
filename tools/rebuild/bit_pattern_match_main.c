/* The test harness for the rebuilt `bit_pattern_match` module.
 *
 * The harness holds the global DAT_000007fd and calls the function with one
 * argument.
 *
 * The paths:
 *
 *     0  the global 0xaf and the argument 0x2f: the six low bits match
 *     1  the global 0xaf and the argument 0x0f: the four low bits match
 *     2  the global 0xaf and the argument 0x1f: the five low bits differ
 *     3  the argument 0: the empty mask matches
 *     4  the global 0 and the argument 0: the empty mask matches a zero global
 *     5  the global 0xffff and the argument 0xff: the eight low bits match
 *     6  the global 0xffff and the argument 0x100: the ninth bit differs
 *     7  the global 0x40000000 and the argument 0x40000000: the high bit matches
 *        after the 31-step mask build
 *     D  the exit marker
 */
#pragma off (check_stack)

int bit_pattern_match(int value);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

int DAT_000007fd;

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

static void check(int number, int ok)
{
    put(ok ? (char)('0' + number) : 'x');
}

int main(void)
{
    DAT_000007fd = 0xaf;
    check(0, bit_pattern_match(0x2f) == 1);
    check(1, bit_pattern_match(0x0f) == 1);
    check(2, bit_pattern_match(0x1f) == 0);
    check(3, bit_pattern_match(0) == 1);

    DAT_000007fd = 0;
    check(4, bit_pattern_match(0) == 1);

    DAT_000007fd = 0xffff;
    check(5, bit_pattern_match(0xff) == 1);
    check(6, bit_pattern_match(0x100) == 0);

    DAT_000007fd = 0x40000000;
    check(7, bit_pattern_match(0x40000000) == 1);

    put('D');
    quit();
    return 0;
}
