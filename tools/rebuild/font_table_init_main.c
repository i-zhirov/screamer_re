/* The test harness for the rebuilt `font_table_init` module.
 *
 * The harness holds the two font table pointers.  The module sets the
 * descriptor table pointer at 0x1ca98 and the bitmap table pointer at 0x19898.
 * The harness fills both cells with a sentinel before each call, so every path
 * checks that the function writes both cells.
 *
 * The harness runs these paths:
 *
 *     0  the sentinel fill, then one call: both pointers hold the constants
 *     1  another fill and call: the second call writes the same values
 *     2  two calls in a row: the values stay constant
 *     D  the exit marker
 *
 * Every path checks the descriptor pointer, the bitmap pointer, and the
 * distinct table addresses.
 */
#pragma off (check_stack)

void font_table_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

static char text[8];
unsigned char *DAT_00015434;
unsigned char *DAT_00015438;

static void put(char marker)
{
    text[0] = marker;
    text[1] = 13;
    text[2] = 10;
    text[3] = '$';
    text[4] = 0;
    print(text);
}

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(void)
{
    DAT_00015434 = (unsigned char *)0x1111;
    DAT_00015438 = (unsigned char *)0x2222;
}

static int pointers_ok(void)
{
    return DAT_00015438 == (unsigned char *)0x1ca98
        && DAT_00015434 == (unsigned char *)0x19898
        && DAT_00015438 != DAT_00015434;
}

int main(void)
{
    reset();
    font_table_init();
    check('0', pointers_ok());

    reset();
    font_table_init();
    check('1', pointers_ok());

    font_table_init();
    font_table_init();
    check('2', pointers_ok());

    put('D');
    quit();
    return 0;
}
