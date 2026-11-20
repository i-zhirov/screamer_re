/* The test harness for the rebuilt `sprite_list_init` module.
 *
 * The harness holds the list base pointer DAT_00015cd0.  The module stores the
 * constant 0x1cb14 in the pointer.
 *
 * The harness runs these paths:
 *
 *     0  the sentinel fill, then one call: the pointer holds 0x1cb14
 *     1  another fill and call: the second call writes the same value
 *     2  two calls in a row: the value stays constant
 *     D  the exit marker
 *
 * Every path checks the pointer.
 */
#pragma off (check_stack)

void sprite_list_init(void);
void print(char *text);
#pragma aux print = "mov ah, 9" "int 0x21" parm [dx] modify exact [ax];
void quit(void);
#pragma aux quit = "mov ax, 0x4C00" "int 0x21" parm [] modify exact [ax];

void *DAT_00015cd0;

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

static void check(char marker, int condition)
{
    put(condition ? marker : 'x');
}

static void reset(void)
{
    DAT_00015cd0 = (void *)0x1111;
}

static int pointer_ok(void)
{
    return DAT_00015cd0 == (void *)0x1cb14;
}

int main(void)
{
    reset();
    sprite_list_init();
    check('0', pointer_ok());

    reset();
    sprite_list_init();
    check('1', pointer_ok());

    sprite_list_init();
    sprite_list_init();
    check('2', pointer_ok());

    put('D');
    quit();
    return 0;
}
